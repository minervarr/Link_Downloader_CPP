#ifndef UTEC_DOWNLOADER_IYTDLPDOWNLOADER_HPP
#define UTEC_DOWNLOADER_IYTDLPDOWNLOADER_HPP

#include <string>
#include <functional>
#include <memory>
#include <filesystem>

namespace utec_downloader {

/**
 * @brief Progress information for downloads
 */
struct DownloadProgress {
    double percentage{0.0};      ///< Progress 0-100
    std::string speed;           ///< Download speed (e.g., "1.5 MiB/s")
    std::string eta;             ///< Estimated time remaining
    std::string filename;        ///< Current filename
    bool finished{false};        ///< Download completed
    std::string error;           ///< Error message if failed
};

/**
 * @brief Callback type for progress updates
 */
using ProgressCallback = std::function<void(const DownloadProgress&)>;

/**
 * @brief Interface for yt-dlp downloaders
 *
 * Defines the contract for video downloading implementations,
 * allowing for different backends (subprocess, native library, etc.)
 */
class IYtdlpDownloader {
public:
    virtual ~IYtdlpDownloader() = default;

    /**
     * @brief Downloads a video from the given URL
     * @param url Video URL
     * @param outputPath Where to save the file
     * @param cookiesFile Optional cookies file for authentication
     * @param quality Video quality setting
     * @param callback Progress callback function
     * @return true if download succeeded
     */
    [[nodiscard]] virtual bool download(
        const std::string& url,
        const std::filesystem::path& outputPath,
        const std::string& cookiesFile = "",
        const std::string& quality = "best",
        ProgressCallback callback = nullptr) = 0;

    /**
     * @brief Checks if the downloader is available
     * @return true if the downloader can be used
     */
    [[nodiscard]] virtual bool isAvailable() const = 0;

    /**
     * @brief Gets the version of the underlying yt-dlp
     * @return Version string
     */
    [[nodiscard]] virtual std::string getVersion() const = 0;

    /**
     * @brief Stops an ongoing download
     */
    virtual void stop() = 0;
};

/**
 * @brief Factory for creating yt-dlp downloader instances
 */
class YtdlpDownloaderFactory {
public:
    /**
     * @brief Creates a downloader instance
     * @param binaryPath Path to yt-dlp binary (for subprocess implementation)
     * @return Unique pointer to downloader
     */
    [[nodiscard]] static std::unique_ptr<IYtdlpDownloader> create(
        const std::filesystem::path& binaryPath = "");
};

} // namespace utec_downloader

#endif // UTEC_DOWNLOADER_IYTDLPDOWNLOADER_HPP
