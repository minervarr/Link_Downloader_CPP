#ifndef UTEC_DOWNLOADER_LOGGER_HPP
#define UTEC_DOWNLOADER_LOGGER_HPP

#include <string>
#include <memory>
#include <filesystem>

namespace utec_downloader {

/**
 * @brief Log severity levels
 */
enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

/**
 * @brief Singleton logger for application-wide logging
 *
 * Provides thread-safe logging to both console and file with
 * configurable severity levels.
 */
class Logger {
public:
    /**
     * @brief Gets the singleton logger instance
     */
    static Logger& getInstance();

    /**
     * @brief Initializes the logger with a log file
     * @param logPath Path to the log file
     */
    void initialize(const std::filesystem::path& logPath);

    /**
     * @brief Sets the minimum log level
     * @param level Minimum severity level to log
     */
    void setLogLevel(LogLevel level);

    /**
     * @brief Gets the current log level
     */
    [[nodiscard]] LogLevel getLogLevel() const;

    /**
     * @brief Enables/disables console output
     */
    void setConsoleOutput(bool enabled);

    /**
     * @brief Enables/disables file output
     */
    void setFileOutput(bool enabled);

    /**
     * @brief Logs a debug message
     */
    void debug(const std::string& message);

    /**
     * @brief Logs an info message
     */
    void info(const std::string& message);

    /**
     * @brief Logs a warning message
     */
    void warning(const std::string& message);

    /**
     * @brief Logs an error message
     */
    void error(const std::string& message);

    /**
     * @brief Logs a message with the specified level
     */
    void log(LogLevel level, const std::string& message);

    // Delete copy constructor and assignment
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

private:
    Logger();
    ~Logger();

    class Impl;
    std::unique_ptr<Impl> pImpl_;
};

/**
 * @brief Converts LogLevel to string
 */
[[nodiscard]] std::string logLevelToString(LogLevel level);

/**
 * @brief Parses a string to LogLevel
 */
[[nodiscard]] LogLevel stringToLogLevel(const std::string& str);

// Convenience macros for logging
#define LOG_DEBUG(msg) utec_downloader::Logger::getInstance().debug(msg)
#define LOG_INFO(msg) utec_downloader::Logger::getInstance().info(msg)
#define LOG_WARNING(msg) utec_downloader::Logger::getInstance().warning(msg)
#define LOG_ERROR(msg) utec_downloader::Logger::getInstance().error(msg)

} // namespace utec_downloader

#endif // UTEC_DOWNLOADER_LOGGER_HPP
