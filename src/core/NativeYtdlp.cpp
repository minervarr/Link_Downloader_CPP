#include "utec_downloader/core/NativeYtdlp.hpp"
#include "utec_downloader/utils/Logger.hpp"

#include <fstream>

// The Downloader_Multi library is still in development - headers (include/ytdlp/)
// are not yet available. Once the library provides public headers, enable:
//
// #if defined(USE_NATIVE_YTDLP) && defined(HAS_CURL)
// #include <ytdlp/core/youtube_dl.hpp>
// #include <ytdlp/extractor/zoom.hpp>
// #define YTDLP_READY 1
// #endif
//
// For now, all methods return stub implementations that fall back to subprocess.

namespace utec_downloader {

// Pimpl implementation class - placeholder until library headers are available
class NativeYtdlp::Impl {
public:
    Impl() = default;
    ~Impl() = default;
};

NativeYtdlp::NativeYtdlp(const std::string& cookiesFile)
    : pImpl_(std::make_unique<Impl>())
    , cookiesFile_(cookiesFile)
{
    LOG_DEBUG("Native yt-dlp C++ library: Downloader_Multi headers not yet available");
}

NativeYtdlp::~NativeYtdlp() = default;

bool NativeYtdlp::download(
    const std::string& url,
    const std::filesystem::path& outputPath,
    const std::string& cookiesFile,
    const std::string& quality,
    ProgressCallback callback) {

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
}

bool NativeYtdlp::isAvailable() const {
    // Downloader_Multi headers not yet available - not ready for production use
    return false;
}

std::string NativeYtdlp::getVersion() const {
    return "not-ready";
}

void NativeYtdlp::stop() {
    stopRequested_ = true;
}

std::optional<std::string> NativeYtdlp::extractInfo(const std::string& url) {
    (void)url;
    return std::nullopt;
}

std::string NativeYtdlp::selectFormat(const std::string& infoDict,
                                       const std::string& quality) {
    (void)infoDict;
    (void)quality;
    return "";
}

bool NativeYtdlp::downloadDirect(const std::string& url,
                                  const std::filesystem::path& outputPath,
                                  ProgressCallback callback) {
    (void)url;
    (void)outputPath;
    (void)callback;
    return false;
}

} // namespace utec_downloader
