#ifndef UTEC_DOWNLOADER_DOWNLOADMANAGER_HPP
#define UTEC_DOWNLOADER_DOWNLOADMANAGER_HPP

#include "utec_downloader/core/ClassInfo.hpp"
#include "utec_downloader/core/IYtdlpDownloader.hpp"

#include <vector>
#include <map>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <future>
#include <queue>
#include <condition_variable>

namespace utec_downloader {

/**
 * @brief Represents a download task
 */
struct DownloadTask {
    std::string id;                        ///< Unique task identifier
    std::string url;                       ///< Video URL
    std::filesystem::path outputPath;      ///< Destination file path
    ClassInfo classInfo;                   ///< Associated class information
    int priority{0};                       ///< Task priority (higher = more important)
};

/**
 * @brief Download task status
 */
enum class TaskStatus {
    Pending,
    InProgress,
    Completed,
    Failed,
    Skipped
};

/**
 * @brief Extended progress information for a task
 */
struct TaskProgress {
    std::string taskId;
    TaskStatus status{TaskStatus::Pending};
    DownloadProgress downloadProgress;
    int retryCount{0};
    std::string errorMessage;
};

/**
 * @brief Download statistics
 */
struct DownloadStats {
    size_t totalTasks{0};
    size_t completed{0};
    size_t failed{0};
    size_t skipped{0};
    size_t pending{0};
    size_t inProgress{0};
};

/**
 * @brief Manages parallel video downloads
 *
 * Provides a thread pool for concurrent downloads with progress
 * tracking, retry logic, and graceful shutdown capabilities.
 */
class DownloadManager {
public:
    /**
     * @brief Constructs a DownloadManager with the given configuration
     * @param config Configuration map
     */
    explicit DownloadManager(const std::map<std::string, std::string>& config);

    ~DownloadManager();

    // Non-copyable
    DownloadManager(const DownloadManager&) = delete;
    DownloadManager& operator=(const DownloadManager&) = delete;

    /**
     * @brief Downloads multiple videos in parallel
     * @param tasks Vector of download tasks
     * @return true if all downloads succeeded
     */
    bool downloadMultiple(const std::vector<DownloadTask>& tasks);

    /**
     * @brief Gets progress for all tasks
     * @return Map of task ID to progress
     */
    [[nodiscard]] std::map<std::string, TaskProgress> getProgress() const;

    /**
     * @brief Gets current download statistics
     */
    [[nodiscard]] DownloadStats getStats() const;

    /**
     * @brief Stops all ongoing downloads
     */
    void stop();

    /**
     * @brief Checks if the manager is currently downloading
     */
    [[nodiscard]] bool isRunning() const { return running_.load(); }

    /**
     * @brief Sets the number of worker threads
     */
    void setWorkerCount(size_t count);

    /**
     * @brief Sets the maximum retry count
     */
    void setMaxRetries(int count) { maxRetries_ = count; }

    /**
     * @brief Sets a callback for overall progress updates
     */
    using StatsCallback = std::function<void(const DownloadStats&)>;
    void setStatsCallback(StatsCallback callback) { statsCallback_ = std::move(callback); }

private:
    void workerThread();
    bool downloadSingleWithRetries(const DownloadTask& task);
    bool executeDownload(const DownloadTask& task);
    void updateProgress(const std::string& taskId, const TaskProgress& progress);
    void initializeDownloader();

    std::map<std::string, std::string> config_;
    std::unique_ptr<IYtdlpDownloader> downloader_;

    // Thread pool
    std::vector<std::thread> workers_;
    std::queue<DownloadTask> taskQueue_;
    mutable std::mutex queueMutex_;
    std::condition_variable queueCondition_;

    // Progress tracking
    std::map<std::string, TaskProgress> progressMap_;
    mutable std::mutex progressMutex_;

    // Control
    std::atomic<bool> running_{false};
    std::atomic<bool> stopRequested_{false};
    size_t workerCount_{4};
    int maxRetries_{3};

    StatsCallback statsCallback_;
};

} // namespace utec_downloader

#endif // UTEC_DOWNLOADER_DOWNLOADMANAGER_HPP
