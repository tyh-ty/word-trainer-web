#pragma once
#include <string>
#include <fstream>
#include <boost/json.hpp>

struct PetConfig {
    int size = 120;
    float r = 1.0f, g = 0.71f, b = 0.77f;
    int bottomRightMargin = 16;
    std::string style = "aimee";
};

struct ApiConfig {
    std::string endpoint = "http://localhost:8080/v1/chat/completions";
    std::string apiKey = "YOUR_API_KEY_HERE";
    std::string model = "gpt-4o-mini";
    int maxTokens = 300;
    int timeoutMs = 30000;
};

struct MonitorConfig {
    int titlePollMs = 1000;
    int screenshotIntervalS = 60;
    bool screenshotEnabled = true;
    int downscaleWidth = 640;
};

struct ChatConfig {
    int bubbleDurationMs = 5000;
    int maxHistory = 20;
};

struct WeatherConfig {
    std::string location = "Beijing";
};

struct LogConfig {
    std::string level = "info";
    std::string file = "desktoppet.log";
};

struct WordTrainerConfig {
    bool enabled = true;
    int port = 18080;
    std::string webPath;
    std::string recordFile = "word_records.json";
    int reminderIntervalS = 30;
    int reminderCooldownS = 120;
};

struct AppConfig {
    PetConfig pet;
    ApiConfig api;
    MonitorConfig monitor;
    ChatConfig chat;
    WeatherConfig weather;
    LogConfig logging;
    WordTrainerConfig wordTrainer;

    bool load(const std::string& path) {
        std::ifstream file(path);
        if (!file) return false;

        std::string json((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());

        try {
            auto root = boost::json::parse(json);

            if (auto* p = root.as_object().if_contains("pet")) {
                auto& petJ = p->as_object();
                if (auto* v = petJ.if_contains("size")) pet.size = v->to_number<int>();
                if (auto* c = petJ.if_contains("color")) {
                    auto& co = c->as_object();
                    if (auto* rv = co.if_contains("r")) pet.r = rv->to_number<float>() / 255.0f;
                    if (auto* gv = co.if_contains("g")) pet.g = gv->to_number<float>() / 255.0f;
                    if (auto* bv = co.if_contains("b")) pet.b = bv->to_number<float>() / 255.0f;
                }
                if (auto* m = petJ.if_contains("bottom_right_margin_px"))
                    pet.bottomRightMargin = m->to_number<int>();
                if (auto* st = petJ.if_contains("style"))
                    pet.style = std::string(st->as_string());
            }

            if (auto* a = root.as_object().if_contains("api")) {
                auto& apiJ = a->as_object();
                if (auto* v = apiJ.if_contains("endpoint")) api.endpoint = std::string(v->as_string());
                if (auto* v = apiJ.if_contains("api_key")) api.apiKey = std::string(v->as_string());
                if (auto* v = apiJ.if_contains("model")) api.model = std::string(v->as_string());
                if (auto* v = apiJ.if_contains("max_tokens")) api.maxTokens = v->to_number<int>();
                if (auto* v = apiJ.if_contains("timeout_ms")) api.timeoutMs = v->to_number<int>();
            }

            if (auto* m = root.as_object().if_contains("monitoring")) {
                auto& monJ = m->as_object();
                if (auto* v = monJ.if_contains("title_poll_interval_ms")) monitor.titlePollMs = v->to_number<int>();
                if (auto* v = monJ.if_contains("screenshot_interval_s")) monitor.screenshotIntervalS = v->to_number<int>();
                if (auto* v = monJ.if_contains("screenshot_enabled")) monitor.screenshotEnabled = v->as_bool();
                if (auto* v = monJ.if_contains("screenshot_downscale_width")) monitor.downscaleWidth = v->to_number<int>();
            }

            if (auto* c = root.as_object().if_contains("chat")) {
                auto& chatJ = c->as_object();
                if (auto* v = chatJ.if_contains("bubble_duration_ms")) chat.bubbleDurationMs = v->to_number<int>();
                if (auto* v = chatJ.if_contains("max_history")) chat.maxHistory = v->to_number<int>();
            }

            if (auto* w = root.as_object().if_contains("weather")) {
                auto& wj = w->as_object();
                if (auto* v = wj.if_contains("location")) weather.location = std::string(v->as_string());
            }

            if (auto* wt = root.as_object().if_contains("word_trainer")) {
                auto& wordJ = wt->as_object();
                if (auto* v = wordJ.if_contains("enabled")) wordTrainer.enabled = v->as_bool();
                if (auto* v = wordJ.if_contains("port")) wordTrainer.port = v->to_number<int>();
                if (auto* v = wordJ.if_contains("web_path")) wordTrainer.webPath = std::string(v->as_string());
                if (auto* v = wordJ.if_contains("record_file")) wordTrainer.recordFile = std::string(v->as_string());
                if (auto* v = wordJ.if_contains("reminder_interval_s")) wordTrainer.reminderIntervalS = v->to_number<int>();
                if (auto* v = wordJ.if_contains("reminder_cooldown_s")) wordTrainer.reminderCooldownS = v->to_number<int>();
            }

            return true;
        } catch (...) {
            return false;
        }
    }
};
