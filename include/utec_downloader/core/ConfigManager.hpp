#ifndef UTEC_DOWNLOADER_CONFIGMANAGER_HPP
#define UTEC_DOWNLOADER_CONFIGMANAGER_HPP

#include <string>
#include <map>
#include <filesystem>
#include <optional>
#include <stdexcept>

namespace utec_downloader {

/**
 * @brief Exception thrown when configuration is invalid
 */
class ConfigException : public std::runtime_error {
public:
    explicit ConfigException(const std::string& message)
        : std::runtime_error(message) {}
};

/**
 * @brief Manages application configuration from file
 *
 * Handles loading, validating, and providing access to configuration
 * values with type-safe getters and default values.
 */
class ConfigManager {
public:
    /**
     * @brief Constructs a ConfigManager with the specified config file path
     * @param configPath Path to the configuration file (default: config/config.txt)
     */
    explicit ConfigManager(const std::filesystem::path& configPath = "config/config.txt");

    /**
     * @brief Loads configuration from file
     * @return Map of configuration key-value pairs
     * @throws ConfigException if the file cannot be read or is invalid
     */
    std::map<std::string, std::string> loadConfig();

    /**
     * @brief Gets a string configuration value
     * @param key Configuration key
     * @param defaultValue Default value if key not found
     * @return Configuration value or default
     */
    [[nodiscard]] std::string get(const std::string& key,
                                   const std::string& defaultValue = "") const;

    /**
     * @brief Gets an integer configuration value
     * @param key Configuration key
     * @param defaultValue Default value if key not found
     * @return Configuration value or default
     */
    [[nodiscard]] int getInt(const std::string& key, int defaultValue = 0) const;

    /**
     * @brief Gets a boolean configuration value
     * @param key Configuration key
     * @param defaultValue Default value if key not found
     * @return Configuration value or default
     */
    [[nodiscard]] bool getBool(const std::string& key, bool defaultValue = false) const;

    /**
     * @brief Checks if a configuration key exists
     */
    [[nodiscard]] bool hasKey(const std::string& key) const;

    /**
     * @brief Gets the current platform identifier
     */
    [[nodiscard]] std::string getPlatform() const { return platform_; }

    /**
     * @brief Gets all configuration as a map
     */
    [[nodiscard]] const std::map<std::string, std::string>& getAll() const { return config_; }

private:
    void createDefaultConfig();
    std::string resolveBinaryPath(const std::string& binaryName);
    void validateConfig();
    std::string detectPlatform();
    std::filesystem::path getProjectRoot();

    std::map<std::string, std::string> config_;
    std::filesystem::path configPath_;
    std::string platform_;
    bool loaded_{false};
};

} // namespace utec_downloader

#endif // UTEC_DOWNLOADER_CONFIGMANAGER_HPP
