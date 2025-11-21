#include "utec_downloader/core/NativeYtdlp.hpp"
#include "utec_downloader/utils/Logger.hpp"

// The yt-dlp_c-library requires libcurl and is still in early development (Phase 1)
// Include the library headers only when both USE_NATIVE_YTDLP and HAS_CURL are defined
#if defined(USE_NATIVE_YTDLP) && defined(HAS_CURL)
#include <ytdlp/core/youtube_dl.hpp>
#include <ytdlp/core/info_dict.hpp>
#define YTDLP_READY 1
#endif

#include <fstream>

namespace utec_downloader {

// Pimpl implementation class
class NativeYtdlp::Impl {
public:
#ifdef YTDLP_READY
    std::unique_ptr<ytdlp::core::YoutubeDL> ytdlp;
#endif

    Impl() = default;
    ~Impl() = default;
};

NativeYtdlp::NativeYtdlp(const std::string& cookiesFile)
    : pImpl_(std::make_unique<Impl>())
    , cookiesFile_(cookiesFile)
{
#ifdef YTDLP_READY
    ytdlp::core::YoutubeDLParams params;
    params.quiet = true;
    if (!cookiesFile.empty()) {
        params.cookies_file = cookiesFile;
    }
    pImpl_->ytdlp = std::make_unique<ytdlp::core::YoutubeDL>(params);
    LOG_INFO("Native yt-dlp C++ library initialized");
#else
    LOG_DEBUG("Native yt-dlp C++ library: waiting for library to mature (Phase 1)");
#endif
}

NativeYtdlp::~NativeYtdlp() = default;

bool NativeYtdlp::download(
    const std::string& url,
    const std::filesystem::path& outputPath,
    const std::string& cookiesFile,
    const std::string& quality,
    ProgressCallback callback) {

#ifdef YTDLP_READY
    stopRequested_ = false;

    LOG_INFO("Starting native download: " + url);

    // Update cookies if provided
    if (!cookiesFile.empty() && cookiesFile != cookiesFile_ && pImpl_->ytdlp) {
        pImpl_->ytdlp->load_cookies(cookiesFile);
    }

    // Extract video information
    auto infoOpt = extractInfo(url);
    if (!infoOpt) {
        LOG_ERROR("Failed to extract video information");
        if (callback) {
            DownloadProgress progress;
            progress.error = "Failed to extract video information";
            callback(progress);
        }
        return false;
    }

    if (stopRequested_) {
        LOG_INFO("Download cancelled");
        return false;
    }

    // Select format based on quality preference
    std::string downloadUrl = selectFormat(*infoOpt, quality);
    if (downloadUrl.empty()) {
        LOG_ERROR("No suitable format found");
        if (callback) {
            DownloadProgress progress;
            progress.error = "No suitable format found";
            callback(progress);
        }
        return false;
    }

    // Perform the actual download
    bool success = downloadDirect(downloadUrl, outputPath, callback);

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

#else
    // Library not ready - inform caller to use subprocess fallback
    (void)url;
    (void)outputPath;
    (void)cookiesFile;
    (void)quality;

    if (callback) {
        DownloadProgress progress;
        progress.error = "Native yt-dlp library not ready - use subprocess fallback";
        callback(progress);
    }
    return false;
#endif
}

bool NativeYtdlp::isAvailable() const {
#ifdef YTDLP_READY
    return pImpl_ && pImpl_->ytdlp != nullptr;
#else
    // Library is in Phase 1 development - not ready for production use
    return false;
#endif
}

std::string NativeYtdlp::getVersion() const {
#ifdef YTDLP_READY
    return "native-cpp-0.1.0";
#else
    return "not-ready";
#endif
}

void NativeYtdlp::stop() {
    stopRequested_ = true;
}

std::optional<std::string> NativeYtdlp::extractInfo(const std::string& url) {
#ifdef YTDLP_READY
    try {
        LOG_DEBUG("Extracting info for: " + url);
        // TODO: Implement extraction using ytdlp library when extractors are ready
        return std::nullopt;
    } catch (const std::exception& e) {
        LOG_ERROR("Extraction failed: " + std::string(e.what()));
        return std::nullopt;
    }
#else
    (void)url;
    return std::nullopt;
#endif
}

std::string NativeYtdlp::selectFormat(const std::string& infoDict,
                                       const std::string& quality) {
#ifdef YTDLP_READY
    try {
        LOG_DEBUG("Selecting format with quality: " + quality);
        // TODO: Implement format selection when the library supports it
        (void)infoDict;
        return "";
    } catch (const std::exception& e) {
        LOG_ERROR("Format selection failed: " + std::string(e.what()));
        return "";
    }
#else
    (void)infoDict;
    (void)quality;
    return "";
#endif
}

bool NativeYtdlp::downloadDirect(const std::string& url,
                                  const std::filesystem::path& outputPath,
                                  ProgressCallback callback) {
#ifdef YTDLP_READY
    try {
        LOG_DEBUG("Downloading from: " + url);

        if (!pImpl_->ytdlp) {
            return false;
        }

        auto& httpClient = pImpl_->ytdlp->http_client();

        // Create output directory if needed
        if (outputPath.has_parent_path()) {
            std::filesystem::create_directories(outputPath.parent_path());
        }

        // TODO: Implement actual download using the HTTP client
        (void)httpClient;
        (void)callback;
        return false;

    } catch (const std::exception& e) {
        LOG_ERROR("Download failed: " + std::string(e.what()));
        return false;
    }
#else
    (void)url;
    (void)outputPath;
    (void)callback;
    return false;
#endif
}

} // namespace utec_downloader
