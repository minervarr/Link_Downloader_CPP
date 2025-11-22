#include "utec_downloader/core/FileOrganizer.hpp"
#include "utec_downloader/utils/Logger.hpp"

#include <algorithm>
#include <sstream>
#include <iomanip>
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

    // New structure: basePath/semesterId/Semana##/
    std::ostringstream weekFolder;
    weekFolder << "Semana" << std::setfill('0') << std::setw(2) << info.weekNumber;

    std::filesystem::path fullPath = basePath_ / semesterId / weekFolder.str();

    if (!std::filesystem::exists(fullPath)) {
        std::filesystem::create_directories(fullPath);
        LOG_DEBUG("Created directory: " + fullPath.string());
    }

    return fullPath;
}

bool FileOrganizer::fileExists(
    const ClassInfo& info,
    const std::string& semesterId) const {

    std::ostringstream weekFolder;
    weekFolder << "Semana" << std::setfill('0') << std::setw(2) << info.weekNumber;

    std::string filename = generateFilename(info);
    std::filesystem::path fullPath = basePath_ / semesterId / weekFolder.str() / filename;

    return std::filesystem::exists(fullPath);
}

std::string FileOrganizer::cleanSubjectName(const std::string& subject) const {
    std::string clean = subject;

    // Replace accented characters with non-accented equivalents
    static const std::vector<std::pair<std::string, std::string>> accentMap = {
        {"á", "a"}, {"é", "e"}, {"í", "i"}, {"ó", "o"}, {"ú", "u"},
        {"Á", "A"}, {"É", "E"}, {"Í", "I"}, {"Ó", "O"}, {"Ú", "U"},
        {"ñ", "n"}, {"Ñ", "N"}, {"ü", "u"}, {"Ü", "U"}
    };

    for (const auto& [from, to] : accentMap) {
        size_t pos = 0;
        while ((pos = clean.find(from, pos)) != std::string::npos) {
            clean.replace(pos, from.length(), to);
            pos += to.length();
        }
    }

    // Replace problematic characters for filesystem
    static const std::regex invalidChars(R"([<>:"/\\|?*])");
    clean = std::regex_replace(clean, invalidChars, "");

    // Replace spaces with underscores
    std::replace(clean.begin(), clean.end(), ' ', '_');

    // Replace multiple underscores with single
    static const std::regex multipleUnderscores(R"(__+)");
    clean = std::regex_replace(clean, multipleUnderscores, "_");

    // Trim underscores from start and end
    auto start = clean.find_first_not_of("_");
    auto end = clean.find_last_not_of("_");
    if (start != std::string::npos) {
        clean = clean.substr(start, end - start + 1);
    }

    // Truncate if too long (filesystem limitations)
    const size_t maxLength = 80;
    if (clean.size() > maxLength) {
        clean = clean.substr(0, maxLength);
        // Don't end with underscore
        while (!clean.empty() && clean.back() == '_') {
            clean.pop_back();
        }
    }

    return clean;
}

std::string FileOrganizer::generateFilename(const ClassInfo& info) const {
    std::ostringstream oss;

    // Format: fecha_horaInicio_subject_tipo_seccion.mp4
    // Example: 2025-11-19_08-00_Circuitos_Digitales-EL2013_Teoria_AB1.mp4

    // Date
    oss << info.fecha;

    // Time (replace : with -)
    if (!info.horaInicio.empty()) {
        std::string time = info.horaInicio;
        std::replace(time.begin(), time.end(), ':', '-');
        oss << "_" << time;
    }

    // Subject (cleaned)
    oss << "_" << cleanSubjectName(info.subject);

    // Class type (Teoría, Práctica, Laboratorio) - cleaned
    std::string tipo = cleanSeccion(info.tipo);
    if (!tipo.empty()) {
        oss << "_" << tipo;
    }

    // Section code (AB1, CD2) - if different from tipo
    std::string seccion = cleanSeccion(info.seccion);
    if (!seccion.empty() && seccion != tipo) {
        oss << "_" << seccion;
    }

    oss << ".mp4";

    return oss.str();
}

std::string FileOrganizer::cleanSeccion(const std::string& seccion) const {
    std::string clean = seccion;

    // Replace accented characters
    static const std::vector<std::pair<std::string, std::string>> accentMap = {
        {"á", "a"}, {"é", "e"}, {"í", "i"}, {"ó", "o"}, {"ú", "u"},
        {"Á", "A"}, {"É", "E"}, {"Í", "I"}, {"Ó", "O"}, {"Ú", "U"},
        {"ñ", "n"}, {"Ñ", "N"}
    };

    for (const auto& [from, to] : accentMap) {
        size_t pos = 0;
        while ((pos = clean.find(from, pos)) != std::string::npos) {
            clean.replace(pos, from.length(), to);
            pos += to.length();
        }
    }

    // Replace spaces with dashes for readability in section
    std::replace(clean.begin(), clean.end(), ' ', '-');

    // Remove problematic characters
    static const std::regex invalidChars(R"([<>:"/\\|?*])");
    clean = std::regex_replace(clean, invalidChars, "");

    return clean;
}

std::string FileOrganizer::determineIdentifier(const ClassInfo& info) const {
    // This function is kept for backward compatibility but not used in new format
    return cleanSeccion(info.seccion);
}

} // namespace utec_downloader
