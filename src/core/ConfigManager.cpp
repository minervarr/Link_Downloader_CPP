#include "utec_downloader/core/ConfigManager.hpp"
#include "utec_downloader/utils/Logger.hpp"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <vector>

namespace utec_downloader {

ConfigManager::ConfigManager(const std::filesystem::path& configPath)
    : configPath_(configPath)
    , platform_(detectPlatform())
{
}

std::map<std::string, std::string> ConfigManager::loadConfig() {
    if (loaded_) {
        return config_;
    }

    if (!std::filesystem::exists(configPath_)) {
        LOG_INFO("Config file not found, creating default: " + configPath_.string());
        createDefaultConfig();
    }

    std::ifstream file(configPath_);
    if (!file.is_open()) {
        throw ConfigException("Cannot open config file: " + configPath_.string());
    }

    std::string line;
    int lineNumber = 0;

    while (std::getline(file, line)) {
        lineNumber++;

        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Trim whitespace
        auto start = line.find_first_not_of(" \t");
        auto end = line.find_last_not_of(" \t");
        if (start == std::string::npos) continue;
        line = line.substr(start, end - start + 1);

        // Find the = separator
        auto delimPos = line.find('=');
        if (delimPos == std::string::npos) {
            LOG_WARNING("Invalid config line " + std::to_string(lineNumber) + ": " + line);
            continue;
        }

        std::string key = line.substr(0, delimPos);
        std::string value = line.substr(delimPos + 1);

        // Trim key and value
        key.erase(key.find_last_not_of(" \t") + 1);
        key.erase(0, key.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));

        config_[key] = value;
    }

    validateConfig();
    loaded_ = true;

    LOG_INFO("Loaded " + std::to_string(config_.size()) + " configuration values");
    return config_;
}

std::string ConfigManager::get(const std::string& key, const std::string& defaultValue) const {
    auto it = config_.find(key);
    return (it != config_.end()) ? it->second : defaultValue;
}

int ConfigManager::getInt(const std::string& key, int defaultValue) const {
    auto it = config_.find(key);
    if (it == config_.end()) {
        return defaultValue;
    }

    try {
        return std::stoi(it->second);
    } catch (const std::exception&) {
        LOG_WARNING("Invalid integer value for key '" + key + "': " + it->second);
        return defaultValue;
    }
}

bool ConfigManager::getBool(const std::string& key, bool defaultValue) const {
    auto it = config_.find(key);
    if (it == config_.end()) {
        return defaultValue;
    }

    std::string value = it->second;
    std::transform(value.begin(), value.end(), value.begin(), ::tolower);

    if (value == "true" || value == "1" || value == "yes" || value == "on") {
        return true;
    } else if (value == "false" || value == "0" || value == "no" || value == "off") {
        return false;
    }

    LOG_WARNING("Invalid boolean value for key '" + key + "': " + it->second);
    return defaultValue;
}

bool ConfigManager::hasKey(const std::string& key) const {
    return config_.find(key) != config_.end();
}

void ConfigManager::createDefaultConfig() {
    // Create directory if needed
    if (configPath_.has_parent_path()) {
        std::filesystem::create_directories(configPath_.parent_path());
    }

    std::ofstream file(configPath_);
    if (!file.is_open()) {
        throw ConfigException("Cannot create config file: " + configPath_.string());
    }

    file << "# UTEC Downloader Configuration\n";
    file << "# Generated automatically\n\n";
    file << "# Download settings\n";
    file << "download_path=" << getProjectRoot().string() << "/downloads\n";
    file << "max_workers=4\n";
    file << "max_retries=3\n";
    file << "quality=best\n\n";
    file << "# yt-dlp settings\n";
    file << "ytdlp_path=" << resolveBinaryPath("yt-dlp") << "\n";
    file << "cookies_file=\n\n";
    file << "# Logging settings\n";
    file << "log_level=INFO\n";
    file << "log_file=logs/downloader.log\n";

    file.close();
    LOG_INFO("Created default configuration file");
}

std::string ConfigManager::resolveBinaryPath(const std::string& binaryName) {
    // Try common locations
    std::vector<std::string> searchPaths;

    if (platform_ == "windows") {
        searchPaths = {
            "C:\\Program Files\\yt-dlp\\" + binaryName + ".exe",
            "C:\\Users\\" + std::string(std::getenv("USERNAME") ? std::getenv("USERNAME") : "") + "\\AppData\\Local\\yt-dlp\\" + binaryName + ".exe"
        };
    } else {
        searchPaths = {
            "/usr/local/bin/" + binaryName,
            "/usr/bin/" + binaryName,
            std::string(std::getenv("HOME") ? std::getenv("HOME") : "") + "/.local/bin/" + binaryName
        };
    }

    for (const auto& path : searchPaths) {
        if (std::filesystem::exists(path)) {
            return path;
        }
    }

    // Fall back to just the binary name (rely on PATH)
    return binaryName;
}

void ConfigManager::validateConfig() {
    // Ensure essential keys exist with defaults
    if (config_.find("download_path") == config_.end()) {
        config_["download_path"] = getProjectRoot().string() + "/downloads";
    }

    if (config_.find("max_workers") == config_.end()) {
        config_["max_workers"] = "4";
    }

    if (config_.find("max_retries") == config_.end()) {
        config_["max_retries"] = "3";
    }
}

std::string ConfigManager::detectPlatform() {
#ifdef _WIN32
    return "windows";
#elif __APPLE__
    return "macos";
#else
    return "linux";
#endif
}

std::filesystem::path ConfigManager::getProjectRoot() {
    // Try to find project root by looking for CMakeLists.txt or config directory
    auto current = std::filesystem::current_path();

    while (current.has_parent_path()) {
        if (std::filesystem::exists(current / "CMakeLists.txt") ||
            std::filesystem::exists(current / "config")) {
            return current;
        }
        if (current == current.parent_path()) break;
        current = current.parent_path();
    }

    return std::filesystem::current_path();
}

} // namespace utec_downloader
