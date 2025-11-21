#ifndef UTEC_DOWNLOADER_SUBPROCESSYTDLP_HPP
#define UTEC_DOWNLOADER_SUBPROCESSYTDLP_HPP

#include "utec_downloader/core/IYtdlpDownloader.hpp"

#include <filesystem>
#include <atomic>

namespace utec_downloader {

/**
 * @brief Subprocess-based yt-dlp downloader implementation
 *
 * Downloads videos by invoking the yt-dlp binary as a subprocess,
 * parsing its output for progress information.
 */
class SubprocessYtdlp : public IYtdlpDownloader {
public:
    /**
     * @brief Constructs a SubprocessYtdlp with the specified binary path
     * @param binaryPath Path to yt-dlp executable
     */
    explicit SubprocessYtdlp(const std::filesystem::path& binaryPath = "yt-dlp");

    [[nodiscard]] bool download(
        const std::string& url,
        const std::filesystem::path& outputPath,
        const std::string& cookiesFile = "",
        const std::string& quality = "best",
        ProgressCallback callback = nullptr) override;

    [[nodiscard]] bool isAvailable() const override;
    [[nodiscard]] std::string getVersion() const override;
    void stop() override;

private:
    /**
     * @brief Parses a progress line from yt-dlp output
     * @param line Output line from yt-dlp
     * @return Parsed progress information
     */
    [[nodiscard]] DownloadProgress parseProgressLine(const std::string& line) const;

    /**
     * @brief Builds the command line arguments for yt-dlp
     */
    [[nodiscard]] std::vector<std::string> buildArguments(
        const std::string& url,
        const std::filesystem::path& outputPath,
        const std::string& cookiesFile,
        const std::string& quality) const;

    std::filesystem::path binaryPath_;
    std::atomic<bool> stopRequested_{false};
    std::atomic<int> currentPid_{-1};
};

} // namespace utec_downloader

#endif // UTEC_DOWNLOADER_SUBPROCESSYTDLP_HPP
