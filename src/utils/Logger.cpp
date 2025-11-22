#include "utec_downloader/utils/Logger.hpp"

#include <fmt/format.h>
#include <fmt/chrono.h>

#include <fstream>
#include <iostream>
#include <mutex>
#include <chrono>
#include <iomanip>

namespace utec_downloader {

class Logger::Impl {
public:
    Impl() = default;

    void initialize(const std::filesystem::path& logPath) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (logPath.has_parent_path()) {
            std::filesystem::create_directories(logPath.parent_path());
        }

        logFile_.open(logPath, std::ios::app);
        if (!logFile_.is_open()) {
            std::cerr << "Warning: Could not open log file: " << logPath << std::endl;
        }

        initialized_ = true;
    }

    void setLogLevel(LogLevel level) {
        logLevel_ = level;
    }

    LogLevel getLogLevel() const {
        return logLevel_;
    }

    void setConsoleOutput(bool enabled) {
        consoleOutput_ = enabled;
    }

    void setFileOutput(bool enabled) {
        fileOutput_ = enabled;
    }

    void log(LogLevel level, const std::string& message) {
        if (level < logLevel_) {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);

        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::tm tm_buf;
#ifdef _WIN32
        localtime_s(&tm_buf, &time);  // Windows: args reversed
#else
        localtime_r(&time, &tm_buf);  // POSIX
#endif
        std::string timestamp = fmt::format("{:%Y-%m-%d %H:%M:%S}.{:03d}",
                                            tm_buf, ms.count());

        std::string levelStr = logLevelToString(level);
        std::string formattedMessage = fmt::format("[{}] [{}] {}",
                                                    timestamp, levelStr, message);

        if (consoleOutput_) {
            // Color output for console
            switch (level) {
                case LogLevel::Error:
                    std::cerr << "\033[31m" << formattedMessage << "\033[0m" << std::endl;
                    break;
                case LogLevel::Warning:
                    std::cout << "\033[33m" << formattedMessage << "\033[0m" << std::endl;
                    break;
                case LogLevel::Info:
                    std::cout << formattedMessage << std::endl;
                    break;
                case LogLevel::Debug:
                    std::cout << "\033[90m" << formattedMessage << "\033[0m" << std::endl;
                    break;
            }
        }

        if (fileOutput_ && logFile_.is_open()) {
            logFile_ << formattedMessage << std::endl;
            logFile_.flush();
        }
    }

private:
    std::mutex mutex_;
    std::ofstream logFile_;
    LogLevel logLevel_{LogLevel::Info};
    bool consoleOutput_{true};
    bool fileOutput_{true};
    bool initialized_{false};
};

Logger::Logger() : pImpl_(std::make_unique<Impl>()) {}

Logger::~Logger() = default;

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::initialize(const std::filesystem::path& logPath) {
    pImpl_->initialize(logPath);
}

void Logger::setLogLevel(LogLevel level) {
    pImpl_->setLogLevel(level);
}

LogLevel Logger::getLogLevel() const {
    return pImpl_->getLogLevel();
}

void Logger::setConsoleOutput(bool enabled) {
    pImpl_->setConsoleOutput(enabled);
}

void Logger::setFileOutput(bool enabled) {
    pImpl_->setFileOutput(enabled);
}

void Logger::debug(const std::string& message) {
    log(LogLevel::Debug, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::Info, message);
}

void Logger::warning(const std::string& message) {
    log(LogLevel::Warning, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::Error, message);
}

void Logger::log(LogLevel level, const std::string& message) {
    pImpl_->log(level, message);
}

std::string logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error:   return "ERROR";
        default:                return "UNKNOWN";
    }
}

LogLevel stringToLogLevel(const std::string& str) {
    if (str == "DEBUG" || str == "debug") return LogLevel::Debug;
    if (str == "INFO" || str == "info") return LogLevel::Info;
    if (str == "WARN" || str == "warn" || str == "WARNING" || str == "warning")
        return LogLevel::Warning;
    if (str == "ERROR" || str == "error") return LogLevel::Error;
    return LogLevel::Info; // Default
}

} // namespace utec_downloader
