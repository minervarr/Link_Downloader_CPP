#include "utec_downloader/core/DataParser.hpp"
#include "utec_downloader/utils/Logger.hpp"

#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>

namespace utec_downloader {

using json = nlohmann::json;

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

        // Handle both array and object formats
        if (data.is_array()) {
            for (const auto& item : data) {
                try {
                    ClassInfo info;
                    info.subject = item.value("subject", item.value("materia", ""));
                    info.fecha = item.value("fecha", item.value("date", ""));
                    info.url = item.value("url", item.value("link", ""));
                    info.weekNumber = item.value("weekNumber", item.value("semana", 0));
                    info.seccion = item.value("seccion", item.value("section", ""));
                    info.modalidad = item.value("modalidad", item.value("modality", ""));

                    if (validateClassData(info)) {
                        result.push_back(std::move(info));
                    } else {
                        LOG_WARNING("Invalid class data in file: " + filePath.string());
                    }
                } catch (const json::exception& e) {
                    LOG_WARNING("Error parsing class entry: " + std::string(e.what()));
                }
            }
        } else if (data.is_object()) {
            // Single object format
            ClassInfo info;
            info.subject = data.value("subject", data.value("materia", ""));
            info.fecha = data.value("fecha", data.value("date", ""));
            info.url = data.value("url", data.value("link", ""));
            info.weekNumber = data.value("weekNumber", data.value("semana", 0));
            info.seccion = data.value("seccion", data.value("section", ""));
            info.modalidad = data.value("modalidad", data.value("modality", ""));

            if (validateClassData(info)) {
                result.push_back(std::move(info));
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
        info.subject = data.value("subject", data.value("materia", ""));
        info.fecha = data.value("fecha", data.value("date", ""));
        info.url = data.value("url", data.value("link", ""));
        info.weekNumber = data.value("weekNumber", data.value("semana", 0));
        info.seccion = data.value("seccion", data.value("section", ""));
        info.modalidad = data.value("modalidad", data.value("modality", ""));
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
