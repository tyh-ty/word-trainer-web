#pragma once
#include "win/PetWindow.h"
#include "pet/Pet.h"
#include "pet/SpeechBubble.h"
#include "monitor/ScreenMonitor.h"
#include "api/ApiClient.h"
#include "util/Config.h"
#include "word/WordTrainerService.h"
#include <Windows.h>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>

class App {
public:
    App() = default;
    ~App() = default;
    int run(HINSTANCE hInstance);

private:
    enum class ApiJobKind {
        Chat,
        Weather
    };

    struct ApiJob {
        ApiJobKind kind = ApiJobKind::Chat;
        std::string systemPrompt;
        std::string userMessage;
        std::string petName;
    };

    AppConfig m_config;
    std::unique_ptr<PetWindow> m_window;
    std::unique_ptr<Pet> m_pet;
    std::unique_ptr<SpeechBubble> m_speechBubble;
    std::unique_ptr<ScreenMonitor> m_monitor;
    std::unique_ptr<ApiClient> m_apiClient;
    std::unique_ptr<WordTrainerService> m_wordTrainerService;
    std::thread m_apiWorker;
    std::mutex m_apiMutex;
    std::condition_variable m_apiCv;
    std::deque<ApiJob> m_apiJobs;
    std::atomic<bool> m_apiWorkerStop{false};
    std::deque<std::string> m_titleHistory;
    mutable std::mutex m_titleMutex;
    int m_clickCount = 0;
    float m_clickTimer = 0;
    float m_chatTimer = 0;
    float m_walkTimer = 8.0f;
    bool m_walking = false;
    bool m_snapToWindow = false;
    POINT m_walkStart = {0, 0};
    POINT m_walkTarget = {0, 0};
    float m_walkProgress = 0;
    HINSTANCE m_hInstance = nullptr;

    void startApiWorker();
    void stopApiWorker();
    void enqueueApiJob(ApiJob job);
    void apiWorkerLoop();
    std::string buildPetPrompt(const std::string& titleUtf8) const;
    std::string buildWeatherPrompt(const std::string& weatherInfo,
                                   const std::string& petName) const;
    std::string generateScreenTalk(const std::string& context);
    std::string petDisplayName() const;
    void handleWordTrainerEvent(const std::string& payload);
};
