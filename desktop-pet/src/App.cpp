#include "App.h"
#include "util/Logger.h"
#include "util/SoundPlayer.h"
#include "util/StringUtils.h"
#include <psapi.h>
#include <mmsystem.h>
#include <shellapi.h>
#include <boost/json.hpp>
#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <utility>

namespace {

std::string timeOfDay() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    int h = st.wHour;
    if (h >= 6 && h < 12) return "现在是早上";
    if (h >= 12 && h < 18) return "现在是下午";
    if (h >= 18 && h < 22) return "现在是傍晚";
    return "现在是深夜";
}

std::string exeDir() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring ws(path);
    auto pos = ws.rfind(L'\\');
    if (pos != std::wstring::npos) ws.resize(pos + 1);
    return StringUtils::wideToUtf8(ws);
}

std::string getEnvUtf8(const wchar_t* name) {
    DWORD size = GetEnvironmentVariableW(name, nullptr, 0);
    if (size == 0) return {};
    std::wstring value(size, L'\0');
    DWORD written = GetEnvironmentVariableW(name, value.data(), size);
    if (written == 0 || written >= size) return {};
    value.resize(written);
    return StringUtils::wideToUtf8(value);
}

bool isAbsolutePath(const std::string& path) {
    if (path.size() >= 3 && std::isalpha(static_cast<unsigned char>(path[0])) &&
        path[1] == ':' && (path[2] == '\\' || path[2] == '/')) {
        return true;
    }
    return path.rfind("\\\\", 0) == 0 || path.rfind("//", 0) == 0;
}

std::string resolveFromBase(const std::string& base, const std::string& path) {
    if (path.empty() || isAbsolutePath(path)) return path;
    return base + path;
}

std::string foregroundTitleUtf8() {
    HWND fg = GetForegroundWindow();
    if (!fg) return {};

    int len = GetWindowTextLengthW(fg);
    if (len <= 0) return {};

    std::wstring title(len + 1, L'\0');
    GetWindowTextW(fg, &title[0], len + 1);
    title.resize(len);
    return StringUtils::wideToUtf8(title);
}

std::string trimAscii(std::string value) {
    auto isSpace = [](unsigned char ch) { return std::isspace(ch) != 0; };
    while (!value.empty() && isSpace(static_cast<unsigned char>(value.front()))) {
        value.erase(value.begin());
    }
    while (!value.empty() && isSpace(static_cast<unsigned char>(value.back()))) {
        value.pop_back();
    }
    return value;
}

std::string cleanBubbleText(std::string value) {
    value.erase(std::remove(value.begin(), value.end(), '\r'), value.end());
    std::replace(value.begin(), value.end(), '\n', ' ');
    value = trimAscii(value);
    while (value.size() >= 2 &&
           ((value.front() == '"' && value.back() == '"') ||
            (value.front() == '\'' && value.back() == '\''))) {
        value = trimAscii(value.substr(1, value.size() - 2));
    }
    if (value.size() > 200) {
        value.resize(200);
    }
    return value;
}

}  // namespace

void App::startApiWorker() {
    m_apiWorkerStop = false;
    m_apiWorker = std::thread(&App::apiWorkerLoop, this);
}

void App::stopApiWorker() {
    m_apiWorkerStop = true;
    m_apiCv.notify_all();
    if (m_apiWorker.joinable()) m_apiWorker.join();
    std::lock_guard<std::mutex> lock(m_apiMutex);
    m_apiJobs.clear();
}

void App::enqueueApiJob(ApiJob job) {
    std::lock_guard<std::mutex> lock(m_apiMutex);
    if (m_apiWorkerStop.load()) return;

    constexpr size_t kMaxQueuedJobs = 3;
    if (m_apiJobs.size() >= kMaxQueuedJobs) {
        if (job.kind == ApiJobKind::Weather) {
            auto it = std::find_if(m_apiJobs.begin(), m_apiJobs.end(),
                [](const ApiJob& item) { return item.kind == ApiJobKind::Chat; });
            if (it != m_apiJobs.end()) {
                m_apiJobs.erase(it);
            } else {
                m_apiJobs.pop_front();
            }
        } else {
            m_apiJobs.pop_front();
        }
    }

    m_apiJobs.push_back(std::move(job));
    m_apiCv.notify_one();
}

std::string App::petDisplayName() const {
    if (!m_pet) return "桌宠";
    switch (m_pet->getStyle()) {
    case PetStyle::Aimee:   return "爱弥斯";
    case PetStyle::Nailong: return "奶龙";
    default:                return "桌宠";
    }
}

std::string App::buildPetPrompt(const std::string& titleUtf8) const {
    std::string historyStr;
    {
        std::lock_guard<std::mutex> lock(m_titleMutex);
        for (const auto& h : m_titleHistory) {
            historyStr += "- " + h + "\n";
        }
    }

    std::string name = petDisplayName();
    std::string prompt = std::string("你是一个可爱的桌面宠物，名字叫") + name + "。";
    prompt += timeOfDay() + "。";
    prompt += "最近用户在看的窗口：\n" + historyStr;
    prompt += "当前窗口：\"" + titleUtf8 + "\"。";
    prompt += "请用中文说一句轻松、俏皮、可爱的评论。";
    prompt += "不要提代码、调试、编程或技术细节。";
    prompt += "回复不超过30个汉字。";
    return prompt;
}

std::string App::buildWeatherPrompt(const std::string& weatherInfo,
                                    const std::string& petName) const {
    if (weatherInfo.empty()) {
        return std::string("你是可爱的桌面宠物") + petName + "。天气接口暂时没有返回结果。"
               "请用中文安慰一句，15个汉字以内。";
    }

    return std::string("你是可爱的桌面宠物") + petName + "。当前天气：" + weatherInfo + "。"
           "请用中文说一句可爱的天气评论，不超过15个汉字。";
}

std::string App::generateScreenTalk(const std::string& context) {
    if (!m_apiClient) return {};

    std::string titleUtf8 = foregroundTitleUtf8();
    if (titleUtf8.empty() && m_monitor) {
        titleUtf8 = StringUtils::wideToUtf8(m_monitor->lastTitle());
    }

    if (!titleUtf8.empty()) {
        std::lock_guard<std::mutex> lock(m_titleMutex);
        if (m_titleHistory.empty() || m_titleHistory.back() != titleUtf8) {
            m_titleHistory.push_back(titleUtf8);
            while ((int)m_titleHistory.size() > m_config.chat.maxHistory) {
                m_titleHistory.pop_front();
            }
        }
    } else {
        titleUtf8 = "未知窗口";
    }

    std::string prompt = buildPetPrompt(titleUtf8);
    if (!context.empty()) {
        prompt += "背单词网页当前状态：" + context + "。";
    }
    prompt += "这句话会显示在网页伴学桌宠气泡里，像真的桌宠闲聊，不要像系统提示。";

    std::string response = m_apiClient->send(
        prompt,
        "根据当前屏幕和学习状态，随机说一句自然可爱的短话。");
    return cleanBubbleText(response);
}

std::wstring localTrainerUrl(int port) {
    std::wostringstream stream;
    stream << L"http://127.0.0.1:" << port << L"/";
    return stream.str();
}

void App::handleWordTrainerEvent(const std::string& payload) {
    try {
        auto root = boost::json::parse(payload).as_object();
        std::string event = root.if_contains("event")
            ? std::string(root.at("event").as_string())
            : std::string();
        std::string message = root.if_contains("message")
            ? std::string(root.at("message").as_string())
            : std::string();

        if (event == "sleep") {
            if (m_pet) {
                m_pet->setMood(PetMood::Sleepy);
                m_pet->playAction(PetAction::Nod);
            }
            if (m_speechBubble) m_speechBubble->show(message.empty() ? "我先睡会儿" : message, 4500);
        } else if (event == "hatch") {
            if (m_pet) {
                m_pet->setMood(PetMood::Surprised);
                m_pet->playAction(PetAction::Hop);
            }
            if (m_speechBubble) m_speechBubble->show(message.empty() ? "第一枚单词破壳啦" : message, 4200);
        } else if (event == "hungry") {
            if (m_pet) {
                m_pet->setMood(PetMood::Curious);
                m_pet->playAction(PetAction::Wave);
            }
            if (m_speechBubble) m_speechBubble->show(message.empty() ? "四小时没学习啦" : message, 5200);
        } else if (event == "happy") {
            if (m_pet) {
                m_pet->setMood(PetMood::Happy);
                m_pet->playAction(PetAction::Cheer);
            }
            if (m_speechBubble) m_speechBubble->show(message.empty() ? "连续答对5题！" : message, 3600);
        } else if (event == "evolve") {
            if (m_pet) {
                m_pet->setMood(PetMood::Happy);
                m_pet->playAction(PetAction::Spin);
            }
            if (m_speechBubble) m_speechBubble->show(message.empty() ? "词汇量进化啦" : message, 5200);
        } else if (event == "leave") {
            if (m_pet) {
                m_pet->setMood(PetMood::Sleepy);
                m_pet->playAction(PetAction::Wave);
            }
            if (m_speechBubble) m_speechBubble->show(message.empty() ? "两天没见，我离家出走啦" : message, 6200);
        } else if (event == "read" || event == "screen") {
            if (m_pet) {
                m_pet->setMood(PetMood::Thinking);
                m_pet->playAction(PetAction::Nod);
            }
            if (!message.empty() && m_speechBubble) m_speechBubble->show(message, 3200);
        } else if (event == "cmd") {
            if (message == "mood_idle" && m_pet) m_pet->setMood(PetMood::Idle);
            else if (message == "mood_happy" && m_pet) m_pet->setMood(PetMood::Happy);
            else if (message == "mood_curious" && m_pet) m_pet->setMood(PetMood::Curious);
            else if (message == "mood_sleepy" && m_pet) m_pet->setMood(PetMood::Sleepy);
            else if (message == "mood_surprised" && m_pet) m_pet->setMood(PetMood::Surprised);
            else if (message == "mood_thinking" && m_pet) m_pet->setMood(PetMood::Thinking);
            else if (message == "pin_bottom" && m_window) m_window->pinToBottomRight(m_config.pet.bottomRightMargin);
            else if (message == "snap_window") {
                m_snapToWindow = !m_snapToWindow;
                if (m_speechBubble) m_speechBubble->show(m_snapToWindow ? "跟随窗口" : "自由闲逛", 1800);
            }
            else if (message == "style_default" && m_pet) {
                m_pet->setStyle(PetStyle::Default);
                if (m_speechBubble) m_speechBubble->show("切回默认外观", 1800);
            }
            else if (message == "style_nailong" && m_pet) {
                m_pet->setStyle(PetStyle::Nailong);
                if (m_speechBubble) m_speechBubble->show("奶龙上线", 1800);
            }
            else if (message == "style_aimee" && m_pet) {
                m_pet->setStyle(PetStyle::Aimee);
                if (m_speechBubble) m_speechBubble->show("爱弥斯上线", 1800);
            }
            else if (message == "clean_memory") {
                PROCESS_MEMORY_COUNTERS pmc;
                GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
                SIZE_T before = pmc.WorkingSetSize;
                SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
                GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
                SIZE_T after = pmc.WorkingSetSize;
                std::ostringstream ss;
                ss << "释放 " << (before - after) / 1024 << "KB";
                if (m_speechBubble) m_speechBubble->show(ss.str(), 2500);
            }
            else if (message == "exit") PostQuitMessage(0);
        }
    } catch (const std::exception& e) {
        LOG_WARN("Bad word trainer event: %s", e.what());
    }
}

void App::apiWorkerLoop() {
    while (true) {
        ApiJob job;
        {
            std::unique_lock<std::mutex> lock(m_apiMutex);
            m_apiCv.wait(lock, [this] {
                return m_apiWorkerStop.load() || !m_apiJobs.empty();
            });

            if (m_apiWorkerStop.load()) {
                break;
            }

            if (m_apiJobs.empty()) {
                continue;
            }

            job = std::move(m_apiJobs.front());
            m_apiJobs.pop_front();
        }

        if (!m_apiClient) continue;

        std::string response;
        if (job.kind == ApiJobKind::Weather) {
            std::string weatherInfo = m_apiClient->fetchWeather(m_config.weather.location);
            std::string prompt = buildWeatherPrompt(weatherInfo, job.petName);
            response = m_apiClient->send(prompt, job.userMessage.empty() ? "天气怎么样？" : job.userMessage);
            if (response.empty()) {
                response = weatherInfo.empty() ? "天气接口开小差了" : weatherInfo;
            }
        } else {
            response = m_apiClient->send(job.systemPrompt, job.userMessage);
        }

        if (m_apiWorkerStop.load() || response.empty() || !m_window) continue;

        auto* msg = new std::string(std::move(response));
        if (!PostMessageW(m_window->getHwnd(), PetWindow::WM_API_RESPONSE,
                          0, reinterpret_cast<LPARAM>(msg))) {
            delete msg;
        }
    }
}

int App::run(HINSTANCE hInstance) {
    timeBeginPeriod(1);
    m_hInstance = hInstance;
    std::string base = exeDir();

    m_config.load(base + "config.json");
    auto& cfg = m_config;

    std::string envApiKey = getEnvUtf8(L"DESKTOPPET_API_KEY");
    if (!envApiKey.empty()) {
        cfg.api.apiKey = envApiKey;
        LOG_INFO("Using API key from DESKTOPPET_API_KEY");
    }
    if (cfg.api.apiKey == "YOUR_API_KEY_HERE") {
        cfg.api.apiKey.clear();
    }

    Logger::instance().setFile(base + cfg.logging.file);
    LOG_INFO("DesktopPet starting...");

    m_window = std::make_unique<PetWindow>(hInstance);
    if (!m_window->create()) {
        LOG_ERROR("Failed to create window");
        timeEndPeriod(1);
        return -1;
    }

    m_pet = std::make_unique<Pet>(cfg.pet.size, cfg.pet.r, cfg.pet.g, cfg.pet.b);

    auto* renderer = m_window->getRenderer();
    ID2D1Bitmap* nailongBitmap = renderer->loadBitmapFromFile(
        StringUtils::utf8ToWide(base + "res/nailong_pet_hd.png"));
    if (!nailongBitmap) {
        nailongBitmap = renderer->loadBitmapFromFile(
            StringUtils::utf8ToWide(base + "res/nailong_pet.png"));
    }
    if (!nailongBitmap) {
        nailongBitmap = renderer->loadBitmapFromFile(
            StringUtils::utf8ToWide(base + "res/nailong.png"));
    }
    if (nailongBitmap) {
        m_pet->setImageNailong(nailongBitmap);
    } else {
        LOG_WARN("Failed to load nailong sprite");
    }

    ID2D1Bitmap* nailongShadow = renderer->loadBitmapFromFile(
        StringUtils::utf8ToWide(base + "res/nailong_pet_shadow.png"));
    if (nailongShadow) {
        m_pet->setImageNailongShadow(nailongShadow);
    }

    ID2D1Bitmap* aimeeBitmap = renderer->loadBitmapFromFile(
        StringUtils::utf8ToWide(base + "res/aimee_pet_hd.png"));
    if (!aimeeBitmap) {
        aimeeBitmap = renderer->loadBitmapFromFile(
            StringUtils::utf8ToWide(base + "res/aimee_pet.png"));
    }
    if (!aimeeBitmap) {
        aimeeBitmap = renderer->loadBitmapFromFile(
            StringUtils::utf8ToWide(base + "res/aimee.png"));
    }
    if (aimeeBitmap) {
        m_pet->setImageAimee(aimeeBitmap);
    } else {
        LOG_WARN("Failed to load aimee sprite");
    }

    ID2D1Bitmap* aimeeShadow = renderer->loadBitmapFromFile(
        StringUtils::utf8ToWide(base + "res/aimee_pet_shadow.png"));
    if (aimeeShadow) {
        m_pet->setImageAimeeShadow(aimeeShadow);
    }

    std::string style = cfg.pet.style;
    if (style == "nailong" || style == "奶龙") {
        m_pet->setStyle(PetStyle::Nailong);
    } else if (style == "aimee" || style == "aimis" || style == "爱弥斯") {
        m_pet->setStyle(PetStyle::Aimee);
    }

    m_monitor = std::make_unique<ScreenMonitor>(
        cfg.monitor.titlePollMs, cfg.monitor.screenshotIntervalS,
        cfg.monitor.downscaleWidth, cfg.monitor.screenshotEnabled);

    if (!cfg.api.apiKey.empty()) {
        m_apiClient = std::make_unique<ApiClient>(
            cfg.api.endpoint, cfg.api.apiKey, cfg.api.model,
            cfg.api.maxTokens, cfg.api.timeoutMs);
    } else {
        LOG_WARN("API key is empty; remote chat and weather comments are disabled");
    }

    m_speechBubble = std::make_unique<SpeechBubble>();

    m_window->loadPosition((base + "pet_position.txt").c_str());

    m_wordTrainerService = std::make_unique<WordTrainerService>(
        m_window->getHwnd(), cfg.wordTrainer, base,
        [this](const std::string& context) {
            return generateScreenTalk(context);
        },
        [this]() -> std::string {
            std::string title = foregroundTitleUtf8();
            if (title.empty() && m_monitor) {
                title = StringUtils::wideToUtf8(m_monitor->lastTitle());
            }
            return title;
        },
        [this]() -> std::string {
            if (!m_pet) return "default";
            switch (m_pet->getStyle()) {
                case PetStyle::Nailong: return "nailong";
                case PetStyle::Aimee: return "aimee";
                default: return "default";
            }
        });
    m_wordTrainerService->start();

    std::string trainerWebPath = resolveFromBase(base, cfg.wordTrainer.webPath);
    m_webView = std::make_unique<WebViewHostWindow>(hInstance);
    m_webView->setRequestHandler([this](const std::string& method,
                                        const std::string& path,
                                        const std::string& body) {
        if (!m_wordTrainerService) {
            return std::string(R"({"ok":false,"error":"word trainer service not ready"})");
        }
        return m_wordTrainerService->handleEmbeddedRequest(method, path, body);
    });
    if (m_webView->create(L"《灵契·伴读灵》--词汇羁绊养成系统",
                          trainerWebPath,
                          base + "webview_user_data")) {
        m_webView->show();
        if (m_speechBubble) m_speechBubble->show("背单词界面已内嵌打开", 2200);
    } else {
        m_webView.reset();
        LOG_WARN("Embedded WebView failed to start");
    }

    startApiWorker();

    m_monitor->setTitleCallback([this](const std::wstring& title) {
        std::string titleUtf8 = StringUtils::wideToUtf8(title);

        {
            std::lock_guard<std::mutex> lock(m_titleMutex);
            m_titleHistory.push_back(titleUtf8);
            while ((int)m_titleHistory.size() > m_config.chat.maxHistory) {
                m_titleHistory.pop_front();
            }
        }

        ApiJob job;
        job.kind = ApiJobKind::Chat;
        job.systemPrompt = buildPetPrompt(titleUtf8);
        job.userMessage = "你觉得我现在在做什么？";
        job.petName = petDisplayName();
        enqueueApiJob(std::move(job));
    });

    m_window->setClickHandler([this](POINT pt) {
        LOG_INFO("Pet clicked at (%d, %d)", pt.x, pt.y);

        if (m_clickTimer > 0) {
            m_clickCount++;
        } else {
            m_clickCount = 1;
        }
        m_clickTimer = 1.0f;

        if (m_clickCount >= 3) {
            m_clickCount = 0;
            m_clickTimer = 0;
            LOG_INFO("Triple-click: fetching weather...");
            SoundPlayer::playMoodChange();
            if (m_pet) m_pet->setMood(PetMood::Curious);

            ApiJob job;
            job.kind = ApiJobKind::Weather;
            job.userMessage = "天气怎么样？";
            job.petName = petDisplayName();
            enqueueApiJob(std::move(job));
            return;
        }

        SoundPlayer::playClick();
        if (m_pet) m_pet->onClick();
    });

    m_window->setMenuCallback([this, base](PetWindow::MenuCmd cmd) {
        if (!m_pet) return;
        switch (cmd) {
        case PetWindow::IDM_MOOD_IDLE:      m_pet->setMood(PetMood::Idle);      break;
        case PetWindow::IDM_MOOD_HAPPY:     m_pet->setMood(PetMood::Happy);     break;
        case PetWindow::IDM_MOOD_CURIOUS:   m_pet->setMood(PetMood::Curious);   break;
        case PetWindow::IDM_MOOD_SLEEPY:    m_pet->setMood(PetMood::Sleepy);    break;
        case PetWindow::IDM_MOOD_SURPRISED: m_pet->setMood(PetMood::Surprised); break;
        case PetWindow::IDM_MOOD_THINKING:  m_pet->setMood(PetMood::Thinking);  break;
        case PetWindow::IDM_PIN_BOTTOM:
            m_window->pinToBottomRight(m_config.pet.bottomRightMargin);
            break;
        case PetWindow::IDM_SNAP_WINDOW:
            m_snapToWindow = !m_snapToWindow;
            if (m_speechBubble) {
                m_speechBubble->show(m_snapToWindow ? "跟随窗口" : "自由闲逛", 1800);
            }
            break;
        case PetWindow::IDM_STYLE_DEFAULT:
            m_pet->setStyle(PetStyle::Default);
            if (m_speechBubble) m_speechBubble->show("切回默认外观", 1800);
            break;
        case PetWindow::IDM_STYLE_NAILONG:
            m_pet->setStyle(PetStyle::Nailong);
            if (m_speechBubble) m_speechBubble->show("奶龙上线", 1800);
            break;
        case PetWindow::IDM_STYLE_AIMEE:
            m_pet->setStyle(PetStyle::Aimee);
            if (m_speechBubble) m_speechBubble->show("爱弥斯上线", 1800);
            break;
        case PetWindow::IDM_OPEN_WORD_TRAINER: {
            if (m_webView && m_webView->getHwnd()) {
                ShowWindow(m_webView->getHwnd(), SW_SHOWNORMAL);
                SetForegroundWindow(m_webView->getHwnd());
                if (m_speechBubble) m_speechBubble->show("背单词界面在 C++ 程序里", 1800);
            } else {
                std::wstring url = localTrainerUrl(m_config.wordTrainer.port);
                HINSTANCE result = ShellExecuteW(
                    nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                if ((INT_PTR)result <= 32 && m_speechBubble) {
                    m_speechBubble->show("背单词界面打开失败", 2200);
                }
            }
            break;
        }
        case PetWindow::IDM_SKIN_PINK:
            m_pet->setColor(1.0f, 0.71f, 0.77f);
            break;
        case PetWindow::IDM_SKIN_BLUE:
            m_pet->setColor(0.55f, 0.71f, 0.95f);
            break;
        case PetWindow::IDM_SKIN_GREEN:
            m_pet->setColor(0.55f, 0.90f, 0.65f);
            break;
        case PetWindow::IDM_SKIN_ORANGE:
            m_pet->setColor(1.0f, 0.65f, 0.40f);
            break;
        case PetWindow::IDM_SKIN_PURPLE:
            m_pet->setColor(0.75f, 0.55f, 0.90f);
            break;
        case PetWindow::IDM_CLEAN_MEMORY: {
            PROCESS_MEMORY_COUNTERS pmc;
            GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
            SIZE_T before = pmc.WorkingSetSize;
            SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
            GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
            SIZE_T after = pmc.WorkingSetSize;
            std::ostringstream ss;
            ss << "释放 " << (before - after) / 1024 << "KB";
            if (m_speechBubble) m_speechBubble->show(ss.str(), 2500);
            break;
        }
        case PetWindow::IDM_EXIT:
            PostQuitMessage(0);
            break;
        }
    });

    m_window->setApiResponseCallback([this](const std::string& text) {
        if (m_speechBubble) m_speechBubble->show(text, m_config.chat.bubbleDurationMs);
        if (m_pet) m_pet->setMood(PetMood::Thinking);
    });

    m_window->setWordEventCallback([this](const std::string& payload) {
        handleWordTrainerEvent(payload);
    });

    auto lastTime = std::chrono::steady_clock::now();

    m_window->setRenderCallback([this, lastTime](D2DRenderer* renderer) mutable {
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        if (dt > 0.1f) dt = 1.0f / 60.0f;

        if (m_monitor) m_monitor->update(dt);

        if (m_clickTimer > 0) {
            m_clickTimer -= dt;
            if (m_clickTimer <= 0) m_clickCount = 0;
        }

        m_chatTimer -= dt;
        if (m_chatTimer <= 0 && m_monitor && m_apiClient) {
            m_chatTimer = 3.0f + (float)(std::rand() % 2);
            std::wstring wtitle = m_monitor->lastTitle();
            if (!wtitle.empty()) {
                std::string titleUtf8 = StringUtils::wideToUtf8(wtitle);
                ApiJob job;
                job.kind = ApiJobKind::Chat;
                job.systemPrompt = buildPetPrompt(titleUtf8);
                job.userMessage = "说句可爱的话。";
                job.petName = petDisplayName();
                enqueueApiJob(std::move(job));
            }
        }

        if (m_snapToWindow && m_window && !m_walking) {
            HWND fg = GetForegroundWindow();
            if (fg && fg != m_window->getHwnd()) {
                RECT r;
                GetWindowRect(fg, &r);
                int nx = r.left + (r.right - r.left) / 2 - m_window->getWidth() / 2;
                int ny = r.top - 220;
                m_window->moveTo(nx, ny);
            }
        }

        m_walkTimer -= dt;
        if (m_walkTimer <= 0 && !m_walking && !m_snapToWindow && m_window) {
            RECT r;
            GetWindowRect(m_window->getHwnd(), &r);
            m_walkStart = {r.left, r.top};
            int sw = GetSystemMetrics(SM_CXSCREEN);
            int sh = GetSystemMetrics(SM_CYSCREEN);
            int maxX = std::max(0, sw - m_window->getWidth());
            int maxY = std::max(0, sh - m_window->getHeight());
            m_walkTarget = {maxX > 0 ? std::rand() % maxX : 0,
                            maxY > 0 ? std::rand() % maxY : 0};
            m_walking = true;
            m_walkProgress = 0;
            m_walkTimer = 12.0f + (float)(std::rand() % 20);
        }

        if (m_walking && m_window) {
            m_walkProgress += dt * 0.4f;
            if (m_walkProgress >= 1.0f) {
                m_walkProgress = 1.0f;
                m_walking = false;
            }
            float t = m_walkProgress * m_walkProgress * (3.0f - 2.0f * m_walkProgress);
            int nx = (int)(m_walkStart.x + (m_walkTarget.x - m_walkStart.x) * t);
            int ny = (int)(m_walkStart.y + (m_walkTarget.y - m_walkStart.y) * t);
            m_window->moveTo(nx, ny);
            if (m_pet) m_pet->setWalking(true);
        } else if (m_pet) {
            m_pet->setWalking(false);
        }

        if (m_pet) {
            m_pet->update(dt);
            m_pet->draw(renderer);
        }
        if (m_speechBubble) {
            m_speechBubble->update(dt);
            int winW = renderer->getWidth();
            int winH = renderer->getHeight();
            float cx = (float)winW / 2.0f;
            float petTop = (float)winH * 0.35f;
            m_speechBubble->draw(renderer, cx, petTop);
        }
    });

    m_window->show();

    RegisterHotKey(m_window->getHwnd(), PetWindow::IDH_TOGGLE,
                   MOD_CONTROL | MOD_SHIFT, 'P');

    SetTimer(m_window->getHwnd(), PetWindow::IDT_RENDER, 7, nullptr);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    KillTimer(m_window->getHwnd(), PetWindow::IDT_RENDER);
    UnregisterHotKey(m_window->getHwnd(), PetWindow::IDH_TOGGLE);
    stopApiWorker();
    m_webView.reset();
    m_wordTrainerService.reset();
    m_window->savePosition((base + "pet_position.txt").c_str());
    m_speechBubble.reset();
    m_apiClient.reset();
    m_monitor.reset();
    m_pet.reset();
    m_window.reset();
    LOG_INFO("DesktopPet exited");
    timeEndPeriod(1);
    return (int)msg.wParam;
}
