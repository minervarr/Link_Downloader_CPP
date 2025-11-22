#include "utec_downloader/core/DataParser.hpp"
#include "utec_downloader/utils/Logger.hpp"

#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <regex>

namespace utec_downloader {

using json = nlohmann::json;

namespace {

/**
 * @brief Converts date from DD/MM/YYYY to YYYY-MM-DD format
 * @param date Input date string
 * @return Normalized date in YYYY-MM-DD format, or original if already in that format
 */
std::string normalizeDateFormat(const std::string& date) {
    if (date.empty()) return date;

    // Check if already in YYYY-MM-DD format
    static const std::regex isoFormat(R"(\d{4}-\d{2}-\d{2})");
    if (std::regex_match(date, isoFormat)) {
        return date;
    }

    // Try DD/MM/YYYY format
    static const std::regex dmyFormat(R"((\d{1,2})/(\d{1,2})/(\d{4}))");
    std::smatch match;
    if (std::regex_match(date, match, dmyFormat)) {
        int day = std::stoi(match[1].str());
        int month = std::stoi(match[2].str());
        std::string year = match[3].str();

        std::ostringstream oss;
        oss << year << "-"
            << std::setfill('0') << std::setw(2) << month << "-"
            << std::setfill('0') << std::setw(2) << day;
        return oss.str();
    }

    // Return as-is if no pattern matches
    return date;
}

/**
 * @brief Parse a single recording object from JSON
 */
ClassInfo parseRecordingObject(const json& item) {
    ClassInfo info;

    // Core fields
    info.subject = item.value("subject", item.value("materia", ""));
    info.fecha = normalizeDateFormat(item.value("fecha", item.value("date", "")));
    info.horaInicio = item.value("horaInicio", item.value("startTime", ""));
    info.url = item.value("url", item.value("link", ""));
    info.weekNumber = item.value("weekNumber", item.value("semana", 0));
    info.seccion = item.value("seccion", item.value("section", ""));
    info.modalidad = item.value("modalidad", item.value("modality", ""));

    // New fields from UTEC Extractor
    info.docente = item.value("docente", "");
    info.tipo = item.value("tipo", "");
    info.estado = item.value("estado", "");
    info.title = item.value("title", "");
    info.timestamp = item.value("timestamp", static_cast<int64_t>(0));
    info.buttonId = item.value("buttonId", "");

    return info;
}

/**
 * @brief Parse an array of recordings
 */
std::vector<ClassInfo> parseRecordingsArray(const json& arr) {
    std::vector<ClassInfo> result;
    for (const auto& item : arr) {
        try {
            result.push_back(parseRecordingObject(item));
        } catch (const json::exception& e) {
            LOG_WARNING("Error parsing recording entry: " + std::string(e.what()));
        }
    }
    return result;
}

} // anonymous namespace

std::vector<ClassInfo> DataParser::parseFile(const std::filesystem::path& filePath) {
    std::vector<ClassInfo> result;

    if (!std::filesystem::exists(filePath)) {
        LOG_ERROR("File does not exist: " + filePath.string());
        return result;
    }

    std::ifstream file(filePath);
    if (!file.is_open()) {
        LOG_ERROR("Cannot open file: " + filePath.string());
        return result;
    }

    try {
        json data = json::parse(file);

        // Format 1: Simple array of recordings (single week export)
        if (data.is_array()) {
            LOG_DEBUG("Parsing single week format (array)");
            auto parsed = parseRecordingsArray(data);
            for (auto& info : parsed) {
                if (validateClassData(info)) {
                    result.push_back(std::move(info));
                } else {
                    LOG_WARNING("Invalid class data in file: " + filePath.string());
                }
            }
        }
        // Object formats
        else if (data.is_object()) {
            // Format 2: All weeks WITH metadata
            // Has "weeks" nested object plus metadata fields
            if (data.contains("weeks") && data["weeks"].is_object()) {
                LOG_DEBUG("Parsing all-weeks format with metadata");

                // Log metadata if present
                if (data.contains("periodo")) {
                    LOG_INFO("Periodo: " + data.value("periodo", ""));
                }
                if (data.contains("totalRecordings")) {
                    LOG_INFO("Total recordings in file: " + std::to_string(data.value("totalRecordings", 0)));
                }

                // Parse each week
                const auto& weeks = data["weeks"];
                for (auto it = weeks.begin(); it != weeks.end(); ++it) {
                    int weekNum = 0;
                    try {
                        weekNum = std::stoi(it.key());
                    } catch (...) {
                        LOG_WARNING("Invalid week key: " + it.key());
                        continue;
                    }

                    if (it.value().is_array()) {
                        auto weekRecordings = parseRecordingsArray(it.value());
                        for (auto& info : weekRecordings) {
                            // Use week number from key if not set in object
                            if (info.weekNumber == 0) {
                                info.weekNumber = weekNum;
                            }
                            if (validateClassData(info)) {
                                result.push_back(std::move(info));
                            }
                        }
                    }
                }
            }
            // Format 3: All weeks WITHOUT metadata (direct week-keyed object)
            // Keys are week numbers directly mapping to arrays
            else if (!data.empty()) {
                // Check if first key looks like a week number
                bool isWeekKeyed = false;
                for (auto it = data.begin(); it != data.end(); ++it) {
                    try {
                        std::stoi(it.key());
                        if (it.value().is_array()) {
                            isWeekKeyed = true;
                            break;
                        }
                    } catch (...) {
                        break;
                    }
                }

                if (isWeekKeyed) {
                    LOG_DEBUG("Parsing all-weeks format without metadata");
                    for (auto it = data.begin(); it != data.end(); ++it) {
                        int weekNum = 0;
                        try {
                            weekNum = std::stoi(it.key());
                        } catch (...) {
                            continue;
                        }

                        if (it.value().is_array()) {
                            auto weekRecordings = parseRecordingsArray(it.value());
                            for (auto& info : weekRecordings) {
                                if (info.weekNumber == 0) {
                                    info.weekNumber = weekNum;
                                }
                                if (validateClassData(info)) {
                                    result.push_back(std::move(info));
                                }
                            }
                        }
                    }
                }
                // Single recording object (legacy format)
                else {
                    LOG_DEBUG("Parsing single recording object");
                    auto info = parseRecordingObject(data);
                    if (validateClassData(info)) {
                        result.push_back(std::move(info));
                    }
                }
            }
        }

        LOG_INFO("Parsed " + std::to_string(result.size()) + " classes from " + filePath.filename().string());

    } catch (const json::parse_error& e) {
        LOG_ERROR("JSON parse error in " + filePath.string() + ": " + e.what());
    } catch (const std::exception& e) {
        LOG_ERROR("Error reading file " + filePath.string() + ": " + e.what());
    }

    return result;
}

std::vector<ClassInfo> DataParser::parseMultipleFiles(
    const std::vector<std::filesystem::path>& filePaths) {

    std::vector<ClassInfo> allClasses;

    for (const auto& path : filePaths) {
        auto classes = parseFile(path);
        allClasses.insert(allClasses.end(),
                          std::make_move_iterator(classes.begin()),
                          std::make_move_iterator(classes.end()));
    }

    LOG_INFO("Total classes parsed from " + std::to_string(filePaths.size()) +
             " files: " + std::to_string(allClasses.size()));

    return allClasses;
}

std::vector<ClassInfo> DataParser::removeDuplicates(const std::vector<ClassInfo>& classes) {
    std::vector<ClassInfo> unique;
    unique.reserve(classes.size());

    for (const auto& info : classes) {
        if (seenUrls_.find(info.url) == seenUrls_.end()) {
            seenUrls_.insert(info.url);
            unique.push_back(info);
        }
    }

    size_t duplicates = classes.size() - unique.size();
    if (duplicates > 0) {
        LOG_INFO("Removed " + std::to_string(duplicates) + " duplicate entries");
    }

    return unique;
}

bool DataParser::validateClassData(const ClassInfo& info) {
    return info.isValid();
}

ClassInfo DataParser::parseClassInfo(const std::string& jsonStr) {
    ClassInfo info;

    try {
        json data = json::parse(jsonStr);
        info = parseRecordingObject(data);
    } catch (const json::exception& e) {
        LOG_ERROR("Failed to parse JSON: " + std::string(e.what()));
    }

    return info;
}

std::vector<std::filesystem::path> findJsonFiles(const std::filesystem::path& directory) {
    std::vector<std::filesystem::path> jsonFiles;

    if (!std::filesystem::exists(directory)) {
        LOG_WARNING("Directory does not exist: " + directory.string());
        return jsonFiles;
    }

    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                auto extension = entry.path().extension().string();
                std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
                if (extension == ".json") {
                    jsonFiles.push_back(entry.path());
                }
            }
        }

        // Sort by filename for consistent ordering
        std::sort(jsonFiles.begin(), jsonFiles.end());

        LOG_INFO("Found " + std::to_string(jsonFiles.size()) + " JSON files in " + directory.string());

    } catch (const std::filesystem::filesystem_error& e) {
        LOG_ERROR("Error scanning directory: " + std::string(e.what()));
    }

    return jsonFiles;
}

} // namespace utec_downloader
