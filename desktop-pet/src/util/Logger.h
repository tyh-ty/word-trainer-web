#pragma once
#include <string>
#include <cstdio>

class Logger {
public:
    static Logger& instance() {
        static Logger log;
        return log;
    }

    void setFile(const std::string& path) {
        if (m_file) fclose(m_file);
        m_file = fopen(path.c_str(), "a");
    }

    ~Logger() {
        if (m_file) fclose(m_file);
    }

    template<typename... Args>
    void info(const char* fmt, Args... args) {
        write("[INFO] ", fmt, args...);
    }

    template<typename... Args>
    void warn(const char* fmt, Args... args) {
        write("[WARN] ", fmt, args...);
    }

    template<typename... Args>
    void error(const char* fmt, Args... args) {
        write("[ERROR] ", fmt, args...);
    }

private:
    Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    template<typename... Args>
    void write(const char* prefix, const char* fmt, Args... args) {
        std::string full = prefix + std::string(fmt) + "\n";
        printf(full.c_str(), args...);
        if (m_file) {
            fprintf(m_file, full.c_str(), args...);
            fflush(m_file);
        }
    }

    FILE* m_file = nullptr;
};

#define LOG_INFO(...) Logger::instance().info(__VA_ARGS__)
#define LOG_WARN(...) Logger::instance().warn(__VA_ARGS__)
#define LOG_ERROR(...) Logger::instance().error(__VA_ARGS__)
