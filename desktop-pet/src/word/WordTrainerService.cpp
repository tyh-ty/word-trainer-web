#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <sapi.h>

#include "WordTrainerService.h"
#include "../util/Logger.h"
#include "../util/StringUtils.h"
#include "../win/PetWindow.h"

#include <boost/json.hpp>
#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <thread>
#include <utility>

namespace {

std::int64_t nowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

std::string lowerCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string trim(const std::string& value) {
    const char* ws = " \t\r\n";
    size_t start = value.find_first_not_of(ws);
    if (start == std::string::npos) return {};
    size_t end = value.find_last_not_of(ws);
    return value.substr(start, end - start + 1);
}

std::string urlPathOnly(const std::string& target) {
    size_t q = target.find('?');
    return q == std::string::npos ? target : target.substr(0, q);
}

int hexValue(char ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
}

std::string urlDecodePath(const std::string& value) {
    std::string out;
    out.reserve(value.size());
    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '%' && i + 2 < value.size()) {
            int hi = hexValue(value[i + 1]);
            int lo = hexValue(value[i + 2]);
            if (hi >= 0 && lo >= 0) {
                out.push_back(static_cast<char>((hi << 4) | lo));
                i += 2;
                continue;
            }
        }
        out.push_back(value[i] == '/' ? '\\' : value[i]);
    }
    return out;
}

bool isAbsolutePath(const std::string& path) {
    if (path.size() >= 3 && std::isalpha(static_cast<unsigned char>(path[0])) &&
        path[1] == ':' && (path[2] == '\\' || path[2] == '/')) {
        return true;
    }
    return path.rfind("\\\\", 0) == 0 || path.rfind("//", 0) == 0;
}

std::string parentDir(std::string path) {
    std::replace(path.begin(), path.end(), '/', '\\');
    size_t pos = path.find_last_of('\\');
    if (pos == std::string::npos) return {};
    return path.substr(0, pos + 1);
}

std::string extensionOf(const std::string& path) {
    size_t slash = path.find_last_of("\\/");
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos || (slash != std::string::npos && dot < slash)) return {};
    std::string ext = path.substr(dot);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return ext;
}

std::string mimeTypeForPath(const std::string& path) {
    static const std::map<std::string, std::string> types = {
        {".html", "text/html; charset=utf-8"},
        {".htm", "text/html; charset=utf-8"},
        {".css", "text/css; charset=utf-8"},
        {".js", "application/javascript; charset=utf-8"},
        {".json", "application/json; charset=utf-8"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".ico", "image/x-icon"},
        {".svg", "image/svg+xml"},
        {".webp", "image/webp"}
    };
    auto it = types.find(extensionOf(path));
    return it == types.end() ? "application/octet-stream" : it->second;
}

std::string resolveFromBase(const std::string& base, const std::string& path) {
    if (path.empty() || isAbsolutePath(path)) return path;
    return base + path;
}

std::string recordKey(const std::string& spelling) {
    std::string key = lowerCopy(trim(spelling));
    return key;
}

std::string jsonString(const boost::json::object& obj, const char* name) {
    if (auto* v = obj.if_contains(name); v && v->is_string()) {
        return std::string(v->as_string());
    }
    return {};
}

bool jsonBool(const boost::json::object& obj, const char* name, bool fallback = false) {
    if (auto* v = obj.if_contains(name); v && v->is_bool()) {
        return v->as_bool();
    }
    return fallback;
}

int jsonInt(const boost::json::object& obj, const char* name, int fallback = 0) {
    if (auto* v = obj.if_contains(name); v && v->is_number()) {
        return v->to_number<int>();
    }
    return fallback;
}

std::int64_t jsonInt64(const boost::json::object& obj, const char* name, std::int64_t fallback = 0) {
    if (auto* v = obj.if_contains(name); v && v->is_number()) {
        return v->to_number<std::int64_t>();
    }
    return fallback;
}

std::string readHttpRequest(SOCKET client, std::string& body) {
    std::string request;
    char buffer[4096];
    int contentLength = 0;
    size_t headerEnd = std::string::npos;

    while (request.size() < 128 * 1024) {
        int received = recv(client, buffer, sizeof(buffer), 0);
        if (received <= 0) break;
        request.append(buffer, received);

        headerEnd = request.find("\r\n\r\n");
        if (headerEnd == std::string::npos) {
            continue;
        }

        std::string headers = request.substr(0, headerEnd + 4);
        std::string lowered = lowerCopy(headers);
        size_t p = lowered.find("content-length:");
        if (p != std::string::npos) {
            size_t lineEnd = lowered.find("\r\n", p);
            std::string value = lowered.substr(p + 15, lineEnd - (p + 15));
            contentLength = std::max(0, std::atoi(value.c_str()));
        }

        size_t bodyStart = headerEnd + 4;
        if (request.size() >= bodyStart + static_cast<size_t>(contentLength)) {
            body = request.substr(bodyStart, contentLength);
            return request.substr(0, headerEnd + 4);
        }
    }

    body.clear();
    return request;
}

bool sendAll(SOCKET socket, const std::string& data) {
    size_t sent = 0;
    while (sent < data.size()) {
        int n = send(socket, data.data() + sent, static_cast<int>(data.size() - sent), 0);
        if (n <= 0) return false;
        sent += static_cast<size_t>(n);
    }
    return true;
}

}  // namespace

WordTrainerService::WordTrainerService(HWND notifyWindow,
                                       const WordTrainerConfig& config,
                                       const std::string& baseDir,
                                       ScreenTalkProvider screenTalkProvider,
                                       ScreenInfoProvider screenInfoProvider,
                                       PetStyleProvider petStyleProvider)
    : m_notifyWindow(notifyWindow)
    , m_config(config)
    , m_baseDir(baseDir)
    , m_recordPath(baseDir + config.recordFile)
    , m_screenTalkProvider(std::move(screenTalkProvider))
    , m_screenInfoProvider(std::move(screenInfoProvider))
    , m_petStyleProvider(std::move(petStyleProvider))
    , m_listenSocket(kInvalidSocket) {}

WordTrainerService::~WordTrainerService() {
    stop();
}

void WordTrainerService::start() {
    if (!m_config.enabled || !m_stop.exchange(false)) {
        return;
    }

    loadRecords();
    m_serverThread = std::thread(&WordTrainerService::serverLoop, this);
    m_reminderThread = std::thread(&WordTrainerService::reminderLoop, this);
}

void WordTrainerService::stop() {
    if (m_stop.exchange(true)) {
        return;
    }

    std::uintptr_t socketValue = m_listenSocket.exchange(kInvalidSocket);
    if (socketValue != kInvalidSocket) {
        closesocket(static_cast<SOCKET>(socketValue));
    }

    if (m_serverThread.joinable()) m_serverThread.join();
    if (m_reminderThread.joinable()) m_reminderThread.join();
}

void WordTrainerService::serverLoop() {
    WSADATA wsaData = {};
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        LOG_ERROR("WordTrainerService WSAStartup failed");
        postBubble("背词助手启动失败");
        return;
    }

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        LOG_ERROR("WordTrainerService socket failed: %d", WSAGetLastError());
        WSACleanup();
        postBubble("背词助手启动失败");
        return;
    }

    BOOL reuse = TRUE;
    setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&reuse), sizeof(reuse));

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(static_cast<u_short>(m_config.port));

    if (bind(listenSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        LOG_ERROR("WordTrainerService bind 127.0.0.1:%d failed: %d",
                  m_config.port, WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        postBubble("背词助手端口被占用");
        return;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        LOG_ERROR("WordTrainerService listen failed: %d", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        postBubble("背词助手监听失败");
        return;
    }

    m_listenSocket = static_cast<std::uintptr_t>(listenSocket);
    LOG_INFO("WordTrainerService listening on http://127.0.0.1:%d", m_config.port);
    postBubble("背词网页助手已启动");

    while (!m_stop.load()) {
        SOCKET client = accept(listenSocket, nullptr, nullptr);
        if (client == INVALID_SOCKET) {
            if (!m_stop.load()) {
                LOG_WARN("WordTrainerService accept failed: %d", WSAGetLastError());
            }
            break;
        }
        handleClient(static_cast<std::uintptr_t>(client));
        closesocket(client);
    }

    std::uintptr_t expected = static_cast<std::uintptr_t>(listenSocket);
    if (m_listenSocket.compare_exchange_strong(expected, kInvalidSocket)) {
        closesocket(listenSocket);
    }
    WSACleanup();
}

void WordTrainerService::reminderLoop() {
    while (!m_stop.load()) {
        for (int i = 0; i < 10 && !m_stop.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        if (m_stop.load()) break;

        std::int64_t now = nowMs();
        if (m_lastReminderAt > 0 &&
            now - m_lastReminderAt < static_cast<std::int64_t>(m_config.reminderCooldownS) * 1000) {
            continue;
        }

        std::string message;
        {
            std::lock_guard<std::mutex> lock(m_recordMutex);
            WordRecord* due = findDueRecordLocked(now);
            if (!due) continue;

            message = "该复习 " + due->spelling + " 了";
            if (!due->meaning.empty()) {
                message += "\n" + due->meaning;
            }
            due->marked = false;
            due->dueAt = now + static_cast<std::int64_t>(m_config.reminderIntervalS) * 1000;
            saveRecordsLocked();
        }

        m_lastReminderAt = now;
        postBubble(message);
    }
}

void WordTrainerService::handleClient(std::uintptr_t clientSocket) {
    SOCKET client = static_cast<SOCKET>(clientSocket);
    std::string body;
    std::string request = readHttpRequest(client, body);
    std::string response = handleRequest(request, body);
    sendAll(client, response);
}

std::string WordTrainerService::handleRequest(const std::string& request,
                                              const std::string& body) {
    std::istringstream stream(request);
    std::string method;
    std::string target;
    std::string version;
    stream >> method >> target >> version;

    if (method == "OPTIONS") {
        return emptyResponse();
    }

    std::string path = urlPathOnly(target);
    if (method == "GET" && path == "/status") return handleStatus();
    if (method == "GET" && path == "/due") return handleDue();
    if (method == "GET" && path == "/screen-info") return handleScreenInfo();
    if (method == "POST" && path == "/speak") return handleSpeak(body);
    if (method == "POST" && path == "/bubble") return handleBubble(body);
    if (method == "POST" && path == "/screen-talk") return handleScreenTalk(body);
    if (method == "POST" && path == "/record") return handleRecord(body);
    if (method == "POST" && path == "/study-state") return handleStudyState(body);
    if (method == "POST" && path == "/pet-cmd") return handlePetCmd(body);
    if (method == "GET") return handleStaticFile(path);

    return jsonResponse(R"({"ok":false,"error":"not found"})", "404 Not Found");
}

std::string WordTrainerService::handleEmbeddedRequest(const std::string& method,
                                                      const std::string& path,
                                                      const std::string& body) {
    if (method == "GET" && path == "/status") return responseBody(handleStatus());
    if (method == "GET" && path == "/due") return responseBody(handleDue());
    if (method == "GET" && path == "/screen-info") return responseBody(handleScreenInfo());
    if (method == "POST" && path == "/speak") return responseBody(handleSpeak(body));
    if (method == "POST" && path == "/bubble") return responseBody(handleBubble(body));
    if (method == "POST" && path == "/screen-talk") return responseBody(handleScreenTalk(body));
    if (method == "POST" && path == "/record") return responseBody(handleRecord(body));
    if (method == "POST" && path == "/study-state") return responseBody(handleStudyState(body));
    if (method == "POST" && path == "/pet-cmd") return responseBody(handlePetCmd(body));
    return R"({"ok":false,"error":"not found"})";
}

std::string WordTrainerService::handleStatus() const {
    boost::json::object root;
    root["ok"] = true;
    root["service"] = "DesktopPet Word Assistant";
    root["port"] = m_config.port;
    root["tts"] = true;
    root["reminders"] = true;
    root["screenTalk"] = static_cast<bool>(m_screenTalkProvider);
    if (m_petStyleProvider) {
        root["petStyle"] = m_petStyleProvider();
    } else {
        root["petStyle"] = "default";
    }
    return jsonResponse(boost::json::serialize(root));
}

std::string WordTrainerService::handleScreenInfo() {
    boost::json::object root;
    root["ok"] = true;
    if (m_screenInfoProvider) {
        root["title"] = m_screenInfoProvider();
    } else {
        root["title"] = "";
    }
    return jsonResponse(boost::json::serialize(root));
}

std::string WordTrainerService::handleSpeak(const std::string& body) {
    try {
        auto root = boost::json::parse(body).as_object();
        std::string word = jsonString(root, "word");
        std::string meaning = jsonString(root, "meaning");
        if (word.empty()) {
            return jsonResponse(R"({"ok":false,"error":"word is empty"})", "400 Bad Request");
        }

        postBubble(meaning.empty() ? "我来读：" + word : "我来读：" + word + "\n" + meaning);
        postPetEvent("read", "听我读：" + word);

        boost::json::object response;
        response["ok"] = true;
        response["word"] = word;
        return jsonResponse(boost::json::serialize(response));
    } catch (const std::exception& e) {
        LOG_ERROR("WordTrainerService /speak parse failed: %s", e.what());
        return jsonResponse(R"({"ok":false,"error":"bad json"})", "400 Bad Request");
    }
}

std::string WordTrainerService::handleBubble(const std::string& body) {
    try {
        auto root = boost::json::parse(body).as_object();
        std::string message = jsonString(root, "message");
        if (message.empty()) {
            return jsonResponse(R"({"ok":false,"error":"message is empty"})", "400 Bad Request");
        }

        postBubble(message);
        return jsonResponse(R"({"ok":true})");
    } catch (const std::exception& e) {
        LOG_ERROR("WordTrainerService /bubble parse failed: %s", e.what());
        return jsonResponse(R"({"ok":false,"error":"bad json"})", "400 Bad Request");
    }
}

std::string WordTrainerService::handleScreenTalk(const std::string& body) {
    if (!m_screenTalkProvider) {
        return jsonResponse(R"({"ok":false,"error":"screen talk unavailable"})", "503 Service Unavailable");
    }

    try {
        std::string context;
        if (!body.empty()) {
            auto root = boost::json::parse(body).as_object();
            context = jsonString(root, "context");
        }

        std::string message = m_screenTalkProvider(context);
        message = trim(message);
        if (message.empty()) {
            return jsonResponse(R"({"ok":false,"error":"empty api response"})", "502 Bad Gateway");
        }

        postBubble(message);
        postPetEvent("screen", message);

        boost::json::object response;
        response["ok"] = true;
        response["message"] = message;
        return jsonResponse(boost::json::serialize(response));
    } catch (const std::exception& e) {
        LOG_ERROR("WordTrainerService /screen-talk failed: %s", e.what());
        return jsonResponse(R"({"ok":false,"error":"screen talk failed"})", "500 Internal Server Error");
    }
}

std::string WordTrainerService::handleRecord(const std::string& body) {
    try {
        auto root = boost::json::parse(body).as_object();
        std::string spelling = trim(jsonString(root, "word"));
        if (spelling.empty()) {
            return jsonResponse(R"({"ok":false,"error":"word is empty"})", "400 Bad Request");
        }

        std::string key = recordKey(spelling);
        bool answeredCorrectly = jsonBool(root, "ok", true);
        std::int64_t now = nowMs();

        {
            std::lock_guard<std::mutex> lock(m_recordMutex);
            WordRecord& record = m_records[key];
            record.spelling = spelling;
            record.meaning = jsonString(root, "meaning");
            record.correct = jsonInt(root, "correct", record.correct);
            record.wrong = jsonInt(root, "wrong", record.wrong);
            record.streak = jsonInt(root, "streak", record.streak);
            record.lastSeen = jsonInt64(root, "lastSeen", now);
            record.dueAt = jsonInt64(root, "dueAt", record.dueAt);
            record.marked = jsonBool(root, "marked", record.marked);

            if (!record.marked && record.dueAt <= 0) {
                record.dueAt = now + (answeredCorrectly ? 24LL * 60 * 60 * 1000 : 60LL * 1000);
            }

            if (!answeredCorrectly) {
                postBubble("这个词稍后再练：" + spelling);
            }

            saveRecordsLocked();
        }

        return jsonResponse(R"({"ok":true})");
    } catch (const std::exception& e) {
        LOG_ERROR("WordTrainerService /record parse failed: %s", e.what());
        return jsonResponse(R"({"ok":false,"error":"bad json"})", "400 Bad Request");
    }
}

std::string WordTrainerService::handleStudyState(const std::string& body) {
    try {
        auto root = boost::json::parse(body).as_object();
        std::string event = jsonString(root, "event");
        std::string message = jsonString(root, "message");
        if (event.empty()) {
            return jsonResponse(R"({"ok":false,"error":"event is empty"})", "400 Bad Request");
        }

        postPetEvent(event, message);
        return jsonResponse(R"({"ok":true})");
    } catch (const std::exception& e) {
        LOG_ERROR("WordTrainerService /study-state parse failed: %s", e.what());
        return jsonResponse(R"({"ok":false,"error":"bad json"})", "400 Bad Request");
    }
}

std::string WordTrainerService::handlePetCmd(const std::string& body) {
    try {
        auto root = boost::json::parse(body).as_object();
        std::string action = jsonString(root, "action");
        if (action.empty()) {
            return jsonResponse(R"({"ok":false,"error":"action is empty"})", "400 Bad Request");
        }
        postPetEvent("cmd", action);
        return jsonResponse(R"({"ok":true})");
    } catch (const std::exception& e) {
        LOG_ERROR("WordTrainerService /pet-cmd parse failed: %s", e.what());
        return jsonResponse(R"({"ok":false,"error":"bad json"})", "400 Bad Request");
    }
}

std::string WordTrainerService::handleDue() const {
    std::int64_t now = nowMs();
    boost::json::array dueWords;

    std::lock_guard<std::mutex> lock(m_recordMutex);
    for (const auto& pair : m_records) {
        const WordRecord& record = pair.second;
        if (!record.marked && (record.dueAt <= 0 || record.dueAt > now)) {
            continue;
        }

        boost::json::object item;
        item["word"] = record.spelling;
        item["meaning"] = record.meaning;
        item["correct"] = record.correct;
        item["wrong"] = record.wrong;
        item["dueAt"] = record.dueAt;
        item["marked"] = record.marked;
        dueWords.push_back(item);
        if (dueWords.size() >= 12) break;
    }

    boost::json::object root;
    root["ok"] = true;
    root["words"] = dueWords;
    return jsonResponse(boost::json::serialize(root));
}

std::string WordTrainerService::handleStaticFile(const std::string& path) const {
    std::string indexPath = resolveFromBase(m_baseDir, m_config.webPath);
    if (indexPath.empty()) {
        return jsonResponse(R"({"ok":false,"error":"web path not configured"})", "404 Not Found");
    }

    std::string webRoot = parentDir(indexPath);
    if (webRoot.empty()) {
        return jsonResponse(R"({"ok":false,"error":"bad web path"})", "404 Not Found");
    }

    std::string filePath;
    if (path.empty() || path == "/") {
        filePath = indexPath;
    } else {
        std::string decoded = urlDecodePath(path);
        while (!decoded.empty() && (decoded.front() == '\\' || decoded.front() == '/')) {
            decoded.erase(decoded.begin());
        }
        if (decoded.empty() || decoded.find("..") != std::string::npos ||
            decoded.find(':') != std::string::npos) {
            return jsonResponse(R"({"ok":false,"error":"bad path"})", "400 Bad Request");
        }
        filePath = webRoot + decoded;
    }

    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        return jsonResponse(R"({"ok":false,"error":"not found"})", "404 Not Found");
    }

    std::string body((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
    return staticResponse(body, mimeTypeForPath(filePath));
}

void WordTrainerService::loadRecords() {
    std::ifstream file(m_recordPath);
    if (!file) return;

    std::string json((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
    try {
        auto root = boost::json::parse(json);
        std::lock_guard<std::mutex> lock(m_recordMutex);
        m_records.clear();
        for (const auto& value : root.as_array()) {
            const auto& obj = value.as_object();
            WordRecord record;
            record.spelling = jsonString(obj, "word");
            record.meaning = jsonString(obj, "meaning");
            record.correct = jsonInt(obj, "correct");
            record.wrong = jsonInt(obj, "wrong");
            record.streak = jsonInt(obj, "streak");
            record.lastSeen = jsonInt64(obj, "lastSeen");
            record.dueAt = jsonInt64(obj, "dueAt");
            record.marked = jsonBool(obj, "marked");

            std::string key = recordKey(record.spelling);
            if (!key.empty()) {
                m_records[key] = std::move(record);
            }
        }
    } catch (const std::exception& e) {
        LOG_WARN("Failed to load word records: %s", e.what());
    }
}

void WordTrainerService::saveRecordsLocked() const {
    boost::json::array root;
    for (const auto& pair : m_records) {
        const WordRecord& record = pair.second;
        boost::json::object item;
        item["word"] = record.spelling;
        item["meaning"] = record.meaning;
        item["correct"] = record.correct;
        item["wrong"] = record.wrong;
        item["streak"] = record.streak;
        item["lastSeen"] = record.lastSeen;
        item["dueAt"] = record.dueAt;
        item["marked"] = record.marked;
        root.push_back(item);
    }

    std::ofstream file(m_recordPath, std::ios::trunc);
    if (file) {
        file << boost::json::serialize(root);
    }
}

void WordTrainerService::postBubble(const std::string& text) const {
    if (!m_notifyWindow || text.empty()) return;

    auto* msg = new std::string(text);
    if (!PostMessageW(m_notifyWindow, PetWindow::WM_API_RESPONSE,
                      0, reinterpret_cast<LPARAM>(msg))) {
        delete msg;
    }
}

void WordTrainerService::postPetEvent(const std::string& event,
                                      const std::string& message) const {
    if (!m_notifyWindow || event.empty()) return;

    boost::json::object root;
    root["event"] = event;
    root["message"] = message;
    auto* payload = new std::string(boost::json::serialize(root));
    if (!PostMessageW(m_notifyWindow, PetWindow::WM_WORD_EVENT,
                      0, reinterpret_cast<LPARAM>(payload))) {
        delete payload;
    }
}

bool WordTrainerService::speakWord(const std::string& text) const {
    return true;
}

WordTrainerService::WordRecord* WordTrainerService::findDueRecordLocked(std::int64_t now) {
    WordRecord* best = nullptr;
    for (auto& pair : m_records) {
        WordRecord& record = pair.second;
        bool due = record.marked || (record.dueAt > 0 && record.dueAt <= now);
        if (!due) continue;
        if (!best || record.dueAt < best->dueAt) {
            best = &record;
        }
    }
    return best;
}

std::string WordTrainerService::jsonResponse(const std::string& body,
                                             const std::string& status) {
    std::ostringstream response;
    response << "HTTP/1.1 " << status << "\r\n"
             << "Content-Type: application/json; charset=utf-8\r\n"
             << "Access-Control-Allow-Origin: *\r\n"
             << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
             << "Access-Control-Allow-Headers: Content-Type\r\n"
             << "Connection: close\r\n"
             << "Content-Length: " << body.size() << "\r\n\r\n"
             << body;
    return response.str();
}

std::string WordTrainerService::staticResponse(const std::string& body,
                                               const std::string& contentType) {
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n"
             << "Content-Type: " << contentType << "\r\n"
             << "Access-Control-Allow-Origin: *\r\n"
             << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
             << "Access-Control-Allow-Headers: Content-Type\r\n"
             << "Cache-Control: no-cache\r\n"
             << "Connection: close\r\n"
             << "Content-Length: " << body.size() << "\r\n\r\n"
             << body;
    return response.str();
}

std::string WordTrainerService::emptyResponse(const std::string& status) {
    std::ostringstream response;
    response << "HTTP/1.1 " << status << "\r\n"
             << "Access-Control-Allow-Origin: *\r\n"
             << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
             << "Access-Control-Allow-Headers: Content-Type\r\n"
             << "Connection: close\r\n"
             << "Content-Length: 0\r\n\r\n";
    return response.str();
}

std::string WordTrainerService::responseBody(const std::string& response) {
    size_t pos = response.find("\r\n\r\n");
    if (pos == std::string::npos) return response;
    return response.substr(pos + 4);
}
