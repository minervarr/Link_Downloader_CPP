#ifndef UTEC_DOWNLOADER_NATIVEYTDLP_HPP
#define UTEC_DOWNLOADER_NATIVEYTDLP_HPP

#include "utec_downloader/core/IYtdlpDownloader.hpp"

#include <memory>
#include <atomic>
#include <optional>

namespace utec_downloader {

/**
 * @brief Native C++ yt-dlp downloader implementation
 *
 * Uses the yt-dlp_c-library for pure C++ video downloading
 * without any Python dependencies.
 *
 * NOTE: The yt-dlp_c-library is currently in Phase 1 development.
 * This implementation will be fully functional once the library matures.
 * Until then, the factory will fall back to SubprocessYtdlp.
 */
class NativeYtdlp : public IYtdlpDownloader {
public:
    /**
     * @brief Constructs a NativeYtdlp downloader
     * @param cookiesFile Optional path to cookies file
     */
    explicit NativeYtdlp(const std::string& cookiesFile = "");

    ~NativeYtdlp() override;

    // Non-copyable
    NativeYtdlp(const NativeYtdlp&) = delete;
    NativeYtdlp& operator=(const NativeYtdlp&) = delete;

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
     * @brief Extracts video information from URL
     * @param url Video URL
     * @return JSON info dict, or nullopt on failure
     */
    [[nodiscard]] std::optional<std::string> extractInfo(const std::string& url);

    /**
     * @brief Selects the best format based on quality preference
     * @param infoDict Video information
     * @param quality Quality preference
     * @return Selected format URL
     */
    [[nodiscard]] std::string selectFormat(const std::string& infoDict,
                                            const std::string& quality);

    /**
     * @brief Downloads content from direct URL
     * @param url Direct video URL
     * @param outputPath Output file path
     * @param callback Progress callback
     * @return true if successful
     */
    bool downloadDirect(const std::string& url,
                        const std::filesystem::path& outputPath,
                        ProgressCallback callback);

    // Use pimpl pattern to avoid incomplete type issues
    class Impl;
    std::unique_ptr<Impl> pImpl_;

    std::atomic<bool> stopRequested_{false};
    std::string cookiesFile_;
};

} // namespace utec_downloader

#endif // UTEC_DOWNLOADER_NATIVEYTDLP_HPP
