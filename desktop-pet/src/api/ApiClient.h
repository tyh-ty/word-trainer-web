#pragma once
#include <string>

class ApiClient {
public:
    ApiClient(const std::string& endpoint, const std::string& apiKey,
              const std::string& model, int maxTokens, int timeoutMs);

    std::string send(const std::string& systemPrompt, const std::string& userMessage);
    std::string fetchWeather(const std::string& location);

private:
    std::string buildRequestBody(const std::string& systemPrompt, const std::string& userMessage);
    std::string parseResponse(const std::string& json);
    std::string httpGet(const std::wstring& host, const std::wstring& path, bool useHttps);

    std::string m_endpoint;
    std::string m_apiKey;
    std::string m_model;
    int m_maxTokens;
    int m_timeoutMs;
};
