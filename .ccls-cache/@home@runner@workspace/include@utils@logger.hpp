#pragma once

#include <string>
#include <sstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <vector>
#include <fstream>

namespace MiniOS {

enum class LogLevel {
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3,
    Critical = 4
};

class Logger {
public:
    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    void setLevel(LogLevel level) { minLevel_ = level; }
    LogLevel getLevel() const { return minLevel_; }

    void log(LogLevel level, const std::string& component, const std::string& message) {
        if (level < minLevel_) return;
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << "[" << std::put_time(std::localtime(&time), "%H:%M:%S")
           << "." << std::setfill('0') << std::setw(3) << ms.count() << "] "
           << "[" << levelToString(level) << "] "
           << "[" << component << "] "
           << message;
        
        std::string logEntry = ss.str();
        logHistory_.push_back(logEntry);
        
        if (consoleOutput_) {
            std::cout << logEntry << std::endl;
        }
    }

    void enableConsoleOutput(bool enable) { consoleOutput_ = enable; }

    const std::vector<std::string>& getHistory() const { return logHistory_; }

    void clearHistory() { 
        std::lock_guard<std::mutex> lock(mutex_);
        logHistory_.clear(); 
    }

    void dumpToFile(const std::string& filename) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::ofstream file(filename);
        for (const auto& entry : logHistory_) {
            file << entry << "\n";
        }
    }

private:
    Logger() : minLevel_(LogLevel::Info), consoleOutput_(true) {}
    
    static const char* levelToString(LogLevel level) {
        switch (level) {
            case LogLevel::Debug: return "DEBUG";
            case LogLevel::Info: return "INFO";
            case LogLevel::Warning: return "WARN";
            case LogLevel::Error: return "ERROR";
            case LogLevel::Critical: return "CRIT";
            default: return "UNKNOWN";
        }
    }

    LogLevel minLevel_;
    bool consoleOutput_;
    std::vector<std::string> logHistory_;
    std::mutex mutex_;
};

#define KERNEL_LOG(level, component, msg) \
    MiniOS::Logger::instance().log(level, component, msg)

#define LOG_DEBUG(component, msg) KERNEL_LOG(MiniOS::LogLevel::Debug, component, msg)
#define LOG_INFO(component, msg) KERNEL_LOG(MiniOS::LogLevel::Info, component, msg)
#define LOG_WARN(component, msg) KERNEL_LOG(MiniOS::LogLevel::Warning, component, msg)
#define LOG_ERROR(component, msg) KERNEL_LOG(MiniOS::LogLevel::Error, component, msg)
#define LOG_CRITICAL(component, msg) KERNEL_LOG(MiniOS::LogLevel::Critical, component, msg)

}
