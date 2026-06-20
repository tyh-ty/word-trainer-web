#pragma once

#include "../util/Config.h"
#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

struct HWND__;
using HWND = HWND__*;

class WordTrainerService {
public:
    using ScreenTalkProvider = std::function<std::string(const std::string& context)>;
    using ScreenInfoProvider = std::function<std::string()>;
    using PetStyleProvider = std::function<std::string()>;

    WordTrainerService(HWND notifyWindow,
                       const WordTrainerConfig& config,
                       const std::string& baseDir,
                       ScreenTalkProvider screenTalkProvider = {},
                       ScreenInfoProvider screenInfoProvider = {},
                       PetStyleProvider petStyleProvider = {});
    ~WordTrainerService();

    void start();
    void stop();
    std::string handleEmbeddedRequest(const std::string& method,
                                      const std::string& path,
                                      const std::string& body);

private:
    struct WordRecord {
        std::string spelling;
        std::string meaning;
        int correct = 0;
        int wrong = 0;
        int streak = 0;
        std::int64_t lastSeen = 0;
        std::int64_t dueAt = 0;
        bool marked = false;
    };

    void serverLoop();
    void reminderLoop();
    void handleClient(std::uintptr_t clientSocket);
    std::string handleRequest(const std::string& request, const std::string& body);
    std::string handleStatus() const;
    std::string handleSpeak(const std::string& body);
    std::string handleBubble(const std::string& body);
    std::string handleScreenTalk(const std::string& body);
    std::string handleRecord(const std::string& body);
    std::string handleStudyState(const std::string& body);
    std::string handlePetCmd(const std::string& body);
    std::string handleScreenInfo();
    std::string handleDue() const;
    std::string handleStaticFile(const std::string& path) const;

    void loadRecords();
    void saveRecordsLocked() const;
    void postBubble(const std::string& text) const;
    void postPetEvent(const std::string& event, const std::string& message) const;
    bool speakWord(const std::string& text) const;
    WordRecord* findDueRecordLocked(std::int64_t nowMs);

    static std::string jsonResponse(const std::string& body, const std::string& status = "200 OK");
    static std::string staticResponse(const std::string& body, const std::string& contentType);
    static std::string emptyResponse(const std::string& status = "204 No Content");
    static std::string responseBody(const std::string& response);

    HWND m_notifyWindow = nullptr;
    WordTrainerConfig m_config;
    std::string m_baseDir;
    std::string m_recordPath;
    ScreenTalkProvider m_screenTalkProvider;
    ScreenInfoProvider m_screenInfoProvider;
    PetStyleProvider m_petStyleProvider;
    std::thread m_serverThread;
    std::thread m_reminderThread;
    std::atomic<bool> m_stop{true};
    std::atomic<std::uintptr_t> m_listenSocket;
    mutable std::mutex m_recordMutex;
    std::unordered_map<std::string, WordRecord> m_records;
    std::int64_t m_lastReminderAt = 0;

    static constexpr std::uintptr_t kInvalidSocket = static_cast<std::uintptr_t>(~0ull);
};
