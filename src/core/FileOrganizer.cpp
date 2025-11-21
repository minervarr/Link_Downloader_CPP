#include "utec_downloader/core/FileOrganizer.hpp"
#include "utec_downloader/utils/Logger.hpp"

#include <algorithm>
#include <sstream>
#include <regex>

namespace utec_downloader {

FileOrganizer::FileOrganizer(const std::filesystem::path& basePath)
    : basePath_(basePath)
{
    // Create base directory if it doesn't exist
    if (!std::filesystem::exists(basePath_)) {
        std::filesystem::create_directories(basePath_);
        LOG_INFO("Created base download directory: " + basePath_.string());
    }
}

std::filesystem::path FileOrganizer::organizeClass(
    const ClassInfo& info,
    const std::string& semesterId) {

    auto directory = createDirectoryStructure(info, semesterId);
    auto filename = generateFilename(info);

    return directory / filename;
}

std::filesystem::path FileOrganizer::createDirectoryStructure(
    const ClassInfo& info,
    const std::string& semesterId) {

    // Structure: basePath/year/semesterId/subject/
    std::string year = info.getYear();
    std::string cleanSubject = cleanSubjectName(info.subject);

    std::filesystem::path fullPath = basePath_ / year / semesterId / cleanSubject;

    if (!std::filesystem::exists(fullPath)) {
        std::filesystem::create_directories(fullPath);
        LOG_DEBUG("Created directory: " + fullPath.string());
    }

    return fullPath;
}

bool FileOrganizer::fileExists(
    const ClassInfo& info,
    const std::string& semesterId) const {

    std::string year = info.getYear();
    std::string cleanSubject = cleanSubjectName(info.subject);
    std::string filename = generateFilename(info);

    std::filesystem::path fullPath = basePath_ / year / semesterId / cleanSubject / filename;

    return std::filesystem::exists(fullPath);
}

std::string FileOrganizer::cleanSubjectName(const std::string& subject) const {
    std::string clean = subject;

    // Replace problematic characters for filesystem
    static const std::regex invalidChars(R"([<>:"/\\|?*])");
    clean = std::regex_replace(clean, invalidChars, "_");

    // Replace multiple spaces with single space
    static const std::regex multipleSpaces(R"(\s+)");
    clean = std::regex_replace(clean, multipleSpaces, " ");

    // Trim whitespace
    auto start = clean.find_first_not_of(" ");
    auto end = clean.find_last_not_of(" ");
    if (start != std::string::npos) {
        clean = clean.substr(start, end - start + 1);
    }

    // Truncate if too long (filesystem limitations)
    const size_t maxLength = 100;
    if (clean.size() > maxLength) {
        clean = clean.substr(0, maxLength);
    }

    return clean;
}

std::string FileOrganizer::generateFilename(const ClassInfo& info) const {
    std::ostringstream oss;

    // Format: subject_week##_date_identifier.mp4
    oss << cleanSubjectName(info.subject);
    oss << "_semana" << std::setfill('0') << std::setw(2) << info.weekNumber;
    oss << "_" << info.fecha;

    std::string identifier = determineIdentifier(info);
    if (!identifier.empty()) {
        oss << "_" << identifier;
    }

    oss << ".mp4";

    return oss.str();
}

std::string FileOrganizer::determineIdentifier(const ClassInfo& info) const {
    std::ostringstream oss;

    // Add section type
    if (!info.seccion.empty()) {
        std::string seccion = info.seccion;
        // Abbreviate common section types
        if (seccion == "TEORÍA" || seccion == "TEORIA") {
            seccion = "T";
        } else if (seccion == "LABORATORIO") {
            seccion = "L";
        } else if (seccion == "PRÁCTICA" || seccion == "PRACTICA") {
            seccion = "P";
        }
        oss << seccion;
    }

    // Add modality if virtual
    if (!info.modalidad.empty()) {
        std::string modalidad = info.modalidad;
        std::transform(modalidad.begin(), modalidad.end(), modalidad.begin(), ::toupper);
        if (modalidad == "VIRTUAL") {
            if (!oss.str().empty()) oss << "_";
            oss << "V";
        }
    }

    return oss.str();
}

} // namespace utec_downloader
