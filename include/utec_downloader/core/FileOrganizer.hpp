#ifndef UTEC_DOWNLOADER_FILEORGANIZER_HPP
#define UTEC_DOWNLOADER_FILEORGANIZER_HPP

#include "utec_downloader/core/ClassInfo.hpp"

#include <string>
#include <filesystem>

namespace utec_downloader {

/**
 * @brief Organizes downloaded files into a structured directory hierarchy
 *
 * Creates a directory structure based on year, semester, and subject,
 * generating appropriate filenames for each class recording.
 */
class FileOrganizer {
public:
    /**
     * @brief Constructs a FileOrganizer with the specified base path
     * @param basePath Root directory for downloads
     */
    explicit FileOrganizer(const std::filesystem::path& basePath);

    /**
     * @brief Generates the organized file path for a class
     * @param info Class information
     * @param semesterId Semester identifier (e.g., "2024-1")
     * @return Full path where the file should be saved
     */
    [[nodiscard]] std::filesystem::path organizeClass(
        const ClassInfo& info,
        const std::string& semesterId);

    /**
     * @brief Creates the directory structure for a class
     * @param info Class information
     * @param semesterId Semester identifier
     * @return Path to the created directory
     */
    [[nodiscard]] std::filesystem::path createDirectoryStructure(
        const ClassInfo& info,
        const std::string& semesterId);

    /**
     * @brief Checks if a file already exists for this class
     * @param info Class information
     * @param semesterId Semester identifier
     * @return true if file already exists
     */
    [[nodiscard]] bool fileExists(
        const ClassInfo& info,
        const std::string& semesterId) const;

    /**
     * @brief Gets the base download path
     */
    [[nodiscard]] const std::filesystem::path& getBasePath() const { return basePath_; }

    /**
     * @brief Sets a new base path
     */
    void setBasePath(const std::filesystem::path& path) { basePath_ = path; }

private:
    [[nodiscard]] std::string cleanSubjectName(const std::string& subject) const;
    [[nodiscard]] std::string generateFilename(const ClassInfo& info) const;
    [[nodiscard]] std::string determineIdentifier(const ClassInfo& info) const;

    std::filesystem::path basePath_;
};

} // namespace utec_downloader

#endif // UTEC_DOWNLOADER_FILEORGANIZER_HPP
