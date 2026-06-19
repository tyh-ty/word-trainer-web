#include "ApiClient.h"
#include "../util/Logger.h"
#include "../util/StringUtils.h"
#include <windows.h>
#include <winhttp.h>
#include <boost/json.hpp>

namespace {

std::string readResponseBody(HINTERNET request) {
    std::string body;
    DWORD bytesRead = 0;
    char buffer[4096];
    do {
        bytesRead = 0;
        if (!WinHttpReadData(request, buffer, sizeof(buffer), &bytesRead)) break;
        if (bytesRead > 0) body.append(buffer, bytesRead);
    } while (bytesRead > 0);
    return body;
}

}  // namespace

ApiClient::ApiClient(const std::string& endpoint, const std::string& apiKey,
                     const std::string& model, int maxTokens, int timeoutMs)
    : m_endpoint(endpoint)
    , m_apiKey(apiKey)
    , m_model(model)
    , m_maxTokens(maxTokens)
    , m_timeoutMs(timeoutMs) {}

std::string ApiClient::buildRequestBody(const std::string& systemPrompt,
                                        const std::string& userMessage) {
    boost::json::object root;
    root["model"] = m_model;
    root["max_tokens"] = m_maxTokens;

    boost::json::array messages;

    boost::json::object sysMsg;
    sysMsg["role"] = "system";
    sysMsg["content"] = systemPrompt;
    messages.push_back(sysMsg);

    boost::json::object userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = userMessage;
    messages.push_back(userMsg);

    root["messages"] = messages;
    return boost::json::serialize(root);
}

std::string ApiClient::parseResponse(const std::string& json) {
    LOG_INFO("API Raw response: %.500s", json.c_str());
    try {
        auto root = boost::json::parse(json);
        if (auto* err = root.as_object().if_contains("error")) {
            LOG_ERROR("API error: %s",
                      err->as_object().at("message").as_string().c_str());
            return {};
        }

        auto& choices = root.at("choices").as_array();
        if (!choices.empty()) {
            auto& msg = choices[0].at("message");
            return std::string(msg.at("content").as_string());
        }
    } catch (const std::exception& e) {
        LOG_ERROR("JSON parse error: %s", e.what());
    }
    return {};
}

std::string ApiClient::send(const std::string& systemPrompt,
                            const std::string& userMessage) {
    std::string requestJson = buildRequestBody(systemPrompt, userMessage);

    URL_COMPONENTS urlComp = {};
    urlComp.dwStructSize = sizeof(urlComp);
    std::wstring wideUrl = StringUtils::utf8ToWide(m_endpoint);
    urlComp.dwSchemeLength = (DWORD)-1;
    urlComp.dwHostNameLength = (DWORD)-1;
    urlComp.dwUrlPathLength = (DWORD)-1;

    if (!WinHttpCrackUrl(wideUrl.c_str(), 0, 0, &urlComp)) {
        LOG_ERROR("Failed to parse URL: %s", m_endpoint.c_str());
        return {};
    }

    std::wstring host(urlComp.lpszHostName, urlComp.dwHostNameLength);
    std::wstring path(urlComp.lpszUrlPath, urlComp.dwUrlPathLength);
    bool useHttps = (urlComp.nScheme == INTERNET_SCHEME_HTTPS);

    HINTERNET session = WinHttpOpen(L"DesktopPet/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) {
        LOG_ERROR("WinHttpOpen failed");
        return {};
    }
    WinHttpSetTimeouts(session, m_timeoutMs, m_timeoutMs, m_timeoutMs, m_timeoutMs);

    HINTERNET connect = WinHttpConnect(session, host.c_str(),
        urlComp.nPort ? urlComp.nPort : (useHttps ? 443 : 80), 0);
    if (!connect) {
        LOG_ERROR("WinHttpConnect failed");
        WinHttpCloseHandle(session);
        return {};
    }

    DWORD flags = useHttps ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET request = WinHttpOpenRequest(connect, L"POST", path.c_str(),
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!request) {
        LOG_ERROR("WinHttpOpenRequest failed");
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return {};
    }

    std::wstring headers = L"Content-Type: application/json\r\n";
    if (!m_apiKey.empty()) {
        headers += L"Authorization: Bearer " + StringUtils::utf8ToWide(m_apiKey) + L"\r\n";
    }

    if (!WinHttpSendRequest(request, headers.c_str(), (DWORD)headers.size(),
                            (LPVOID)requestJson.c_str(), (DWORD)requestJson.size(),
                            (DWORD)requestJson.size(), 0)) {
        LOG_ERROR("WinHttpSendRequest failed: %lu", GetLastError());
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return {};
    }

    if (!WinHttpReceiveResponse(request, nullptr)) {
        LOG_ERROR("WinHttpReceiveResponse failed: %lu", GetLastError());
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return {};
    }

    std::string responseBody = readResponseBody(request);

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);

    return parseResponse(responseBody);
}

std::string ApiClient::httpGet(const std::wstring& host, const std::wstring& path,
                               bool useHttps) {
    HINTERNET session = WinHttpOpen(L"DesktopPet/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) return {};

    WinHttpSetTimeouts(session, m_timeoutMs, m_timeoutMs, m_timeoutMs, m_timeoutMs);

    HINTERNET connect = WinHttpConnect(session, host.c_str(),
        useHttps ? 443 : 80, 0);
    if (!connect) {
        WinHttpCloseHandle(session);
        return {};
    }

    DWORD flags = useHttps ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET request = WinHttpOpenRequest(connect, L"GET", path.c_str(),
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!request) {
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return {};
    }

    if (!WinHttpSendRequest(request, nullptr, 0, nullptr, 0, 0, 0)) {
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return {};
    }

    if (!WinHttpReceiveResponse(request, nullptr)) {
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return {};
    }

    std::string body = readResponseBody(request);

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);
    return body;
}

std::string ApiClient::fetchWeather(const std::string& location) {
    std::wstring path = L"/" + StringUtils::utf8ToWide(location) + L"?format=j1";

    std::string json = httpGet(L"wttr.in", path, true);
    if (json.empty()) {
        LOG_ERROR("Weather fetch failed: empty response");
        return {};
    }

    LOG_INFO("Weather raw: %.300s", json.c_str());

    try {
        auto root = boost::json::parse(json);
        auto& cc = root.at("current_condition").as_array();
        if (cc.empty()) return {};

        auto& cond = cc[0].as_object();
        std::string temp = std::string(cond.at("temp_C").as_string());
        std::string desc = std::string(cond.at("weatherDesc").as_array()[0].at("value").as_string());
        std::string humidity = std::string(cond.at("humidity").as_string());

        std::string summary = location + " " + temp + "C, " + desc + ", 湿度" + humidity + "%";
        LOG_INFO("Weather summary: %s", summary.c_str());
        return summary;
    } catch (const std::exception& e) {
        LOG_ERROR("Weather parse error: %s", e.what());
        return {};
    }
}
