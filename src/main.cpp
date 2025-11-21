#include "utec_downloader/core/ConfigManager.hpp"
#include "utec_downloader/core/DataParser.hpp"
#include "utec_downloader/core/SemesterClassifier.hpp"
#include "utec_downloader/core/FileOrganizer.hpp"
#include "utec_downloader/core/DownloadManager.hpp"
#include "utec_downloader/utils/Logger.hpp"

#include <fmt/format.h>
#include <iostream>
#include <algorithm>

using namespace utec_downloader;

namespace {

void printBanner() {
    std::cout << R"(
╔═══════════════════════════════════════════════════════════╗
║           UTEC Video Downloader - C++ Edition             ║
║              Modern C++17 Parallel Downloader             ║
╚═══════════════════════════════════════════════════════════╝
)" << std::endl;
}

void printUsage(const char* programName) {
    std::cout << fmt::format("Usage: {} [options]\n\n", programName);
    std::cout << "Options:\n";
    std::cout << "  -c, --config <path>    Path to configuration file\n";
    std::cout << "  -d, --data <path>      Path to JSON data directory\n";
    std::cout << "  -o, --output <path>    Output directory for downloads\n";
    std::cout << "  -w, --workers <n>      Number of parallel workers\n";
    std::cout << "  -v, --verbose          Enable verbose logging\n";
    std::cout << "  -h, --help             Show this help message\n";
}

struct AppConfig {
    std::filesystem::path configPath = "config/config.txt";
    std::filesystem::path dataPath = "data/json_files";
    std::filesystem::path outputPath;
    int workerCount = 0;
    bool verbose = false;
};

AppConfig parseArgs(int argc, char* argv[]) {
    AppConfig config;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            std::exit(0);
        } else if ((arg == "-c" || arg == "--config") && i + 1 < argc) {
            config.configPath = argv[++i];
        } else if ((arg == "-d" || arg == "--data") && i + 1 < argc) {
            config.dataPath = argv[++i];
        } else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            config.outputPath = argv[++i];
        } else if ((arg == "-w" || arg == "--workers") && i + 1 < argc) {
            config.workerCount = std::stoi(argv[++i]);
        } else if (arg == "-v" || arg == "--verbose") {
            config.verbose = true;
        }
    }

    return config;
}

void printStats(const DownloadStats& stats) {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════╗\n";
    std::cout << "║           Download Summary            ║\n";
    std::cout << "╠═══════════════════════════════════════╣\n";
    std::cout << fmt::format("║  Total tasks:     {:>6}              ║\n", stats.totalTasks);
    std::cout << fmt::format("║  Completed:       {:>6}              ║\n", stats.completed);
    std::cout << fmt::format("║  Failed:          {:>6}              ║\n", stats.failed);
    std::cout << fmt::format("║  Skipped:         {:>6}              ║\n", stats.skipped);
    std::cout << "╚═══════════════════════════════════════╝\n";
}

} // anonymous namespace

int main(int argc, char* argv[]) {
    try {
        printBanner();

        // Parse command line arguments
        auto appConfig = parseArgs(argc, argv);

        // Initialize logger
        Logger::getInstance().initialize("logs/downloader.log");
        if (appConfig.verbose) {
            Logger::getInstance().setLogLevel(LogLevel::Debug);
        }

        LOG_INFO("Starting UTEC Video Downloader");

        // Load configuration
        ConfigManager configManager(appConfig.configPath);
        auto config = configManager.loadConfig();

        // Override config with command line args
        if (!appConfig.outputPath.empty()) {
            config["download_path"] = appConfig.outputPath.string();
        }
        if (appConfig.workerCount > 0) {
            config["max_workers"] = std::to_string(appConfig.workerCount);
        }

        // Find JSON files
        auto jsonFiles = findJsonFiles(appConfig.dataPath);
        if (jsonFiles.empty()) {
            LOG_ERROR("No JSON files found in: " + appConfig.dataPath.string());
            std::cerr << "Error: No JSON data files found. Please add class data to "
                      << appConfig.dataPath << std::endl;
            return 1;
        }

        std::cout << fmt::format("Found {} JSON files\n", jsonFiles.size());

        // Parse class data
        DataParser parser;
        auto allClasses = parser.parseMultipleFiles(jsonFiles);

        if (allClasses.empty()) {
            LOG_ERROR("No valid class data found");
            std::cerr << "Error: Could not parse any valid class data\n";
            return 1;
        }

        // Remove duplicates
        auto uniqueClasses = parser.removeDuplicates(allClasses);
        std::cout << fmt::format("Parsed {} unique classes\n", uniqueClasses.size());

        // Validate class data
        size_t invalidCount = 0;
        std::vector<ClassInfo> validClasses;
        validClasses.reserve(uniqueClasses.size());

        for (const auto& classInfo : uniqueClasses) {
            if (classInfo.isValid()) {
                validClasses.push_back(classInfo);
            } else {
                invalidCount++;
                LOG_WARNING("Invalid class data: " + classInfo.subject);
            }
        }

        if (invalidCount > 0) {
            std::cout << fmt::format("Skipped {} invalid entries\n", invalidCount);
        }

        // Initialize semester classifier and file organizer
        SemesterClassifier classifier;
        FileOrganizer organizer(config["download_path"]);

        // Build download tasks
        std::vector<DownloadTask> tasks;
        size_t skippedExisting = 0;

        for (const auto& classInfo : validClasses) {
            std::string semesterId = classifier.classify(classInfo.fecha);

            // Check if file already exists
            if (organizer.fileExists(classInfo, semesterId)) {
                skippedExisting++;
                continue;
            }

            DownloadTask task;
            task.id = classInfo.getUniqueId();
            task.url = classInfo.url;
            task.outputPath = organizer.organizeClass(classInfo, semesterId);
            task.classInfo = classInfo;

            tasks.push_back(std::move(task));
        }

        if (skippedExisting > 0) {
            std::cout << fmt::format("Skipping {} already downloaded files\n", skippedExisting);
        }

        if (tasks.empty()) {
            std::cout << "All files are already downloaded. Nothing to do.\n";
            return 0;
        }

        std::cout << fmt::format("Starting download of {} files...\n\n", tasks.size());

        // Start downloads
        DownloadManager downloadManager(config);

        // Set up progress callback
        downloadManager.setStatsCallback([](const DownloadStats& stats) {
            std::cout << fmt::format("\rProgress: {}/{} completed, {} failed",
                                     stats.completed, stats.totalTasks, stats.failed);
            std::cout.flush();
        });

        bool success = downloadManager.downloadMultiple(tasks);

        // Print final statistics
        printStats(downloadManager.getStats());

        LOG_INFO("Download session completed");

        return success ? 0 : 1;

    } catch (const ConfigException& e) {
        std::cerr << "Configuration error: " << e.what() << std::endl;
        LOG_ERROR("Configuration error: " + std::string(e.what()));
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        LOG_ERROR("Fatal error: " + std::string(e.what()));
        return 1;
    }
}
