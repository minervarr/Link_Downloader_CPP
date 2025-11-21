#ifndef UTEC_DOWNLOADER_DATAPARSER_HPP
#define UTEC_DOWNLOADER_DATAPARSER_HPP

#include "utec_downloader/core/ClassInfo.hpp"

#include <vector>
#include <filesystem>
#include <unordered_set>
#include <memory>

namespace utec_downloader {

/**
 * @brief Parses class information from JSON files
 *
 * Handles parsing, deduplication, and validation of class data
 * from JSON sources.
 */
class DataParser {
public:
    DataParser() = default;

    /**
     * @brief Parses a single JSON file
     * @param filePath Path to the JSON file
     * @return Vector of parsed ClassInfo objects
     */
    [[nodiscard]] std::vector<ClassInfo> parseFile(const std::filesystem::path& filePath);

    /**
     * @brief Parses multiple JSON files
     * @param filePaths Vector of paths to JSON files
     * @return Vector of all parsed ClassInfo objects
     */
    [[nodiscard]] std::vector<ClassInfo> parseMultipleFiles(
        const std::vector<std::filesystem::path>& filePaths);

    /**
     * @brief Removes duplicate entries based on URL
     * @param classes Vector of ClassInfo objects
     * @return Vector with duplicates removed
     */
    [[nodiscard]] std::vector<ClassInfo> removeDuplicates(
        const std::vector<ClassInfo>& classes);

    /**
     * @brief Validates a single ClassInfo object
     * @param info ClassInfo to validate
     * @return true if valid
     */
    [[nodiscard]] static bool validateClassData(const ClassInfo& info);

    /**
     * @brief Gets the list of URLs that have been seen (for deduplication)
     */
    [[nodiscard]] const std::unordered_set<std::string>& getSeenUrls() const {
        return seenUrls_;
    }

    /**
     * @brief Resets the seen URLs set
     */
    void resetSeenUrls() { seenUrls_.clear(); }

private:
    ClassInfo parseClassInfo(const std::string& jsonStr);
    std::unordered_set<std::string> seenUrls_;
};

/**
 * @brief Finds all JSON files in a directory
 * @param directory Directory to search
 * @return Vector of paths to JSON files
 */
[[nodiscard]] std::vector<std::filesystem::path> findJsonFiles(
    const std::filesystem::path& directory);

} // namespace utec_downloader

#endif // UTEC_DOWNLOADER_DATAPARSER_HPP
