#include "utec_downloader/core/DownloadManager.hpp"
#include "utec_downloader/core/SubprocessYtdlp.hpp"
#include "utec_downloader/utils/Logger.hpp"

#include <fmt/format.h>

namespace utec_downloader {

DownloadManager::DownloadManager(const std::map<std::string, std::string>& config)
    : config_(config)
{
    // Get worker count from config
    auto it = config_.find("max_workers");
    if (it != config_.end()) {
        try {
            workerCount_ = std::stoul(it->second);
        } catch (...) {
            workerCount_ = 4;
        }
    }

    // Get max retries from config
    it = config_.find("max_retries");
    if (it != config_.end()) {
        try {
            maxRetries_ = std::stoi(it->second);
        } catch (...) {
            maxRetries_ = 3;
        }
    }

    initializeDownloader();
}

DownloadManager::~DownloadManager() {
    stop();
}

void DownloadManager::initializeDownloader() {
    std::string ytdlpPath = "yt-dlp";
    auto it = config_.find("ytdlp_path");
    if (it != config_.end() && !it->second.empty()) {
        ytdlpPath = it->second;
    }

    downloader_ = YtdlpDownloaderFactory::create(ytdlpPath);

    if (!downloader_->isAvailable()) {
        LOG_WARNING("yt-dlp not available at: " + ytdlpPath);
    } else {
        LOG_INFO("Using yt-dlp version: " + downloader_->getVersion());
    }
}

bool DownloadManager::downloadMultiple(const std::vector<DownloadTask>& tasks) {
    if (tasks.empty()) {
        LOG_INFO("No tasks to download");
        return true;
    }

    running_ = true;
    stopRequested_ = false;

    // Initialize progress tracking
    {
        std::lock_guard<std::mutex> lock(progressMutex_);
        progressMap_.clear();
        for (const auto& task : tasks) {
            TaskProgress progress;
            progress.taskId = task.id;
            progress.status = TaskStatus::Pending;
            progressMap_[task.id] = progress;
        }
    }

    // Add tasks to queue
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        for (const auto& task : tasks) {
            taskQueue_.push(task);
        }
    }

    LOG_INFO(fmt::format("Starting download of {} tasks with {} workers",
                         tasks.size(), workerCount_));

    // Start worker threads
    workers_.clear();
    for (size_t i = 0; i < workerCount_; ++i) {
        workers_.emplace_back(&DownloadManager::workerThread, this);
    }

    // Wait for all workers to complete
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    running_ = false;

    // Calculate results
    auto stats = getStats();
    LOG_INFO(fmt::format("Download complete: {} succeeded, {} failed, {} skipped",
                         stats.completed, stats.failed, stats.skipped));

    return stats.failed == 0;
}

void DownloadManager::workerThread() {
    while (!stopRequested_) {
        DownloadTask task;

        // Get next task from queue
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            if (taskQueue_.empty()) {
                break; // No more tasks
            }
            task = taskQueue_.front();
            taskQueue_.pop();
        }

        // Process the task
        bool success = downloadSingleWithRetries(task);

        // Update progress
        {
            std::lock_guard<std::mutex> lock(progressMutex_);
            auto& progress = progressMap_[task.id];
            progress.status = success ? TaskStatus::Completed : TaskStatus::Failed;
        }

        // Notify stats callback
        if (statsCallback_) {
            statsCallback_(getStats());
        }
    }
}

bool DownloadManager::downloadSingleWithRetries(const DownloadTask& task) {
    // Update status to in progress
    {
        std::lock_guard<std::mutex> lock(progressMutex_);
        progressMap_[task.id].status = TaskStatus::InProgress;
    }

    for (int attempt = 0; attempt <= maxRetries_; ++attempt) {
        if (stopRequested_) {
            return false;
        }

        if (attempt > 0) {
            LOG_INFO(fmt::format("Retry {}/{} for task: {}",
                                 attempt, maxRetries_, task.id));
            // Exponential backoff
            std::this_thread::sleep_for(std::chrono::seconds(1 << attempt));
        }

        if (executeDownload(task)) {
            return true;
        }

        // Update retry count
        {
            std::lock_guard<std::mutex> lock(progressMutex_);
            progressMap_[task.id].retryCount = attempt + 1;
        }
    }

    LOG_ERROR(fmt::format("Failed after {} retries: {}", maxRetries_, task.id));
    return false;
}

bool DownloadManager::executeDownload(const DownloadTask& task) {
    LOG_INFO(fmt::format("Downloading: {} -> {}",
                         task.classInfo.subject, task.outputPath.filename().string()));

    std::string cookiesFile;
    auto it = config_.find("cookies_file");
    if (it != config_.end()) {
        cookiesFile = it->second;
    }

    std::string quality = "best";
    it = config_.find("quality");
    if (it != config_.end()) {
        quality = it->second;
    }

    auto progressCallback = [this, &task](const DownloadProgress& progress) {
        std::lock_guard<std::mutex> lock(progressMutex_);
        auto& taskProgress = progressMap_[task.id];
        taskProgress.downloadProgress = progress;
        if (!progress.error.empty()) {
            taskProgress.errorMessage = progress.error;
        }
    };

    return downloader_->download(
        task.url,
        task.outputPath,
        cookiesFile,
        quality,
        progressCallback
    );
}

std::map<std::string, TaskProgress> DownloadManager::getProgress() const {
    std::lock_guard<std::mutex> lock(progressMutex_);
    return progressMap_;
}

DownloadStats DownloadManager::getStats() const {
    std::lock_guard<std::mutex> lock(progressMutex_);

    DownloadStats stats;
    stats.totalTasks = progressMap_.size();

    for (const auto& [id, progress] : progressMap_) {
        switch (progress.status) {
            case TaskStatus::Pending:
                stats.pending++;
                break;
            case TaskStatus::InProgress:
                stats.inProgress++;
                break;
            case TaskStatus::Completed:
                stats.completed++;
                break;
            case TaskStatus::Failed:
                stats.failed++;
                break;
            case TaskStatus::Skipped:
                stats.skipped++;
                break;
        }
    }

    return stats;
}

void DownloadManager::stop() {
    stopRequested_ = true;

    if (downloader_) {
        downloader_->stop();
    }

    // Wake up any waiting workers
    queueCondition_.notify_all();

    // Wait for workers to finish
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    workers_.clear();
    running_ = false;

    LOG_INFO("Download manager stopped");
}

void DownloadManager::setWorkerCount(size_t count) {
    if (running_) {
        LOG_WARNING("Cannot change worker count while downloading");
        return;
    }
    workerCount_ = count > 0 ? count : 1;
}

void DownloadManager::updateProgress(const std::string& taskId,
                                      const TaskProgress& progress) {
    std::lock_guard<std::mutex> lock(progressMutex_);
    progressMap_[taskId] = progress;
}

} // namespace utec_downloader
