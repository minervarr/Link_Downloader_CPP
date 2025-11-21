#include "utec_downloader/core/SubprocessYtdlp.hpp"
#include "utec_downloader/utils/Logger.hpp"

#ifdef USE_NATIVE_YTDLP
#include "utec_downloader/core/NativeYtdlp.hpp"
#endif

#include <array>
#include <cstdio>
#include <regex>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#endif

namespace utec_downloader {

SubprocessYtdlp::SubprocessYtdlp(const std::filesystem::path& binaryPath)
    : binaryPath_(binaryPath)
{
}

bool SubprocessYtdlp::download(
    const std::string& url,
    const std::filesystem::path& outputPath,
    const std::string& cookiesFile,
    const std::string& quality,
    ProgressCallback callback) {

    stopRequested_ = false;

    auto args = buildArguments(url, outputPath, cookiesFile, quality);

    // Build command string
    std::ostringstream cmdStream;
    cmdStream << binaryPath_.string();
    for (const auto& arg : args) {
        // Quote arguments with spaces
        if (arg.find(' ') != std::string::npos) {
            cmdStream << " \"" << arg << "\"";
        } else {
            cmdStream << " " << arg;
        }
    }
    cmdStream << " 2>&1";

    std::string command = cmdStream.str();
    LOG_DEBUG("Executing: " + command);

    // Execute command and capture output
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        LOG_ERROR("Failed to execute yt-dlp command");
        if (callback) {
            DownloadProgress progress;
            progress.error = "Failed to execute command";
            callback(progress);
        }
        return false;
    }

    std::array<char, 256> buffer;
    std::string line;
    bool success = true;

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        if (stopRequested_) {
            pclose(pipe);
            LOG_INFO("Download stopped by user");
            return false;
        }

        line = buffer.data();

        // Remove trailing newline
        if (!line.empty() && line.back() == '\n') {
            line.pop_back();
        }

        // Parse progress and call callback
        if (callback && !line.empty()) {
            auto progress = parseProgressLine(line);
            callback(progress);

            if (!progress.error.empty()) {
                success = false;
            }
        }
    }

    int status = pclose(pipe);
    bool commandSucceeded = (status == 0);

    if (!commandSucceeded) {
        LOG_ERROR("yt-dlp command failed with status: " + std::to_string(status));
        success = false;
    }

    // Verify file was created
    if (success && !std::filesystem::exists(outputPath)) {
        // yt-dlp might add extension, check for common patterns
        std::string basePath = outputPath.string();
        if (basePath.size() > 4) {
            basePath = basePath.substr(0, basePath.size() - 4); // Remove .mp4
        }

        bool found = false;
        for (const auto& ext : {".mp4", ".mkv", ".webm", ".m4a"}) {
            if (std::filesystem::exists(basePath + ext)) {
                found = true;
                break;
            }
        }

        if (!found) {
            LOG_WARNING("Output file not found after download");
            success = false;
        }
    }

    if (callback) {
        DownloadProgress finalProgress;
        finalProgress.finished = success;
        finalProgress.percentage = success ? 100.0 : 0.0;
        if (!success) {
            finalProgress.error = "Download failed";
        }
        callback(finalProgress);
    }

    return success;
}

bool SubprocessYtdlp::isAvailable() const {
    std::string command = binaryPath_.string() + " --version 2>&1";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return false;
    }

    std::array<char, 128> buffer;
    std::string result;
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }

    int status = pclose(pipe);
    return status == 0 && !result.empty();
}

std::string SubprocessYtdlp::getVersion() const {
    std::string command = binaryPath_.string() + " --version 2>&1";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return "unknown";
    }

    std::array<char, 128> buffer;
    std::string result;
    if (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result = buffer.data();
        // Remove trailing newline
        if (!result.empty() && result.back() == '\n') {
            result.pop_back();
        }
    }

    pclose(pipe);
    return result.empty() ? "unknown" : result;
}

void SubprocessYtdlp::stop() {
    stopRequested_ = true;

#ifndef _WIN32
    if (currentPid_ > 0) {
        kill(currentPid_, SIGTERM);
    }
#endif
}

DownloadProgress SubprocessYtdlp::parseProgressLine(const std::string& line) const {
    DownloadProgress progress;

    // Pattern: [download]  XX.X% of ~XXX.XXMiB at XXXKiB/s ETA XX:XX
    static const std::regex progressRegex(
        R"(\[download\]\s+(\d+\.?\d*)%\s+of\s+~?[\d.]+\w+\s+at\s+([\d.]+\w+/s)\s+ETA\s+(\S+))"
    );

    // Pattern: [download] Destination: filename
    static const std::regex destRegex(R"(\[download\]\s+Destination:\s+(.+))");

    // Pattern: ERROR: ...
    static const std::regex errorRegex(R"(ERROR:\s*(.+))");

    std::smatch match;

    if (std::regex_search(line, match, progressRegex)) {
        progress.percentage = std::stod(match[1].str());
        progress.speed = match[2].str();
        progress.eta = match[3].str();
    } else if (std::regex_search(line, match, destRegex)) {
        progress.filename = match[1].str();
    } else if (std::regex_search(line, match, errorRegex)) {
        progress.error = match[1].str();
        LOG_ERROR("yt-dlp error: " + progress.error);
    } else if (line.find("[download] 100%") != std::string::npos) {
        progress.percentage = 100.0;
        progress.finished = true;
    }

    return progress;
}

std::vector<std::string> SubprocessYtdlp::buildArguments(
    const std::string& url,
    const std::filesystem::path& outputPath,
    const std::string& cookiesFile,
    const std::string& quality) const {

    std::vector<std::string> args;

    // Output template
    args.push_back("-o");
    args.push_back(outputPath.string());

    // Quality selection
    args.push_back("-f");
    args.push_back(quality);

    // Don't overwrite existing files
    args.push_back("--no-overwrites");

    // Continue partial downloads
    args.push_back("-c");

    // Cookies file if provided
    if (!cookiesFile.empty() && std::filesystem::exists(cookiesFile)) {
        args.push_back("--cookies");
        args.push_back(cookiesFile);
    }

    // Progress output
    args.push_back("--newline");

    // URL last
    args.push_back(url);

    return args;
}

// Factory implementation
std::unique_ptr<IYtdlpDownloader> YtdlpDownloaderFactory::create(
    const std::filesystem::path& binaryPath) {

#ifdef USE_NATIVE_YTDLP
    // Try native C++ implementation first
    auto native = std::make_unique<NativeYtdlp>();
    if (native->isAvailable()) {
        LOG_INFO("Using native C++ yt-dlp implementation");
        return native;
    }
    LOG_INFO("Native yt-dlp not ready, falling back to subprocess");
#endif

    // Fallback to subprocess implementation
    std::filesystem::path path = binaryPath.empty() ? "yt-dlp" : binaryPath;
    return std::make_unique<SubprocessYtdlp>(path);
}

} // namespace utec_downloader
