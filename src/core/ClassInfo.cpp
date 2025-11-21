#include "utec_downloader/core/ClassInfo.hpp"

#include <regex>
#include <algorithm>
#include <sstream>

namespace utec_downloader {

bool ClassInfo::isValid() const noexcept {
    return !subject.empty() &&
           !fecha.empty() &&
           !url.empty() &&
           weekNumber > 0;
}

std::string ClassInfo::getYear() const noexcept {
    if (fecha.size() >= 4) {
        return fecha.substr(0, 4);
    }
    return "";
}

std::string ClassInfo::getUniqueId() const {
    std::ostringstream oss;
    oss << subject << "_" << fecha << "_" << seccion << "_" << weekNumber;
    return oss.str();
}

bool ClassInfo::operator==(const ClassInfo& other) const noexcept {
    return subject == other.subject &&
           fecha == other.fecha &&
           url == other.url &&
           weekNumber == other.weekNumber &&
           seccion == other.seccion &&
           modalidad == other.modalidad;
}

bool ClassInfo::operator!=(const ClassInfo& other) const noexcept {
    return !(*this == other);
}

namespace validation {

bool isValidDate(const std::string& date) {
    // Pattern: YYYY-MM-DD
    static const std::regex dateRegex(R"(\d{4}-\d{2}-\d{2})");

    if (!std::regex_match(date, dateRegex)) {
        return false;
    }

    // Extract and validate components
    int year = std::stoi(date.substr(0, 4));
    int month = std::stoi(date.substr(5, 2));
    int day = std::stoi(date.substr(8, 2));

    if (year < 2000 || year > 2100) return false;
    if (month < 1 || month > 12) return false;
    if (day < 1 || day > 31) return false;

    // Days per month (simplified, not accounting for leap years)
    static const int daysInMonth[] = {0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return day <= daysInMonth[month];
}

bool isValidUrl(const std::string& url) {
    if (url.empty()) return false;

    // Basic URL validation
    static const std::regex urlRegex(
        R"(^https?://[^\s/$.?#].[^\s]*$)",
        std::regex::icase
    );

    return std::regex_match(url, urlRegex);
}

std::vector<std::string> validateClassInfo(const ClassInfo& info) {
    std::vector<std::string> errors;

    if (info.subject.empty()) {
        errors.push_back("Subject is empty");
    }

    if (info.fecha.empty()) {
        errors.push_back("Date (fecha) is empty");
    } else if (!isValidDate(info.fecha)) {
        errors.push_back("Invalid date format (expected YYYY-MM-DD): " + info.fecha);
    }

    if (info.url.empty()) {
        errors.push_back("URL is empty");
    } else if (!isValidUrl(info.url)) {
        errors.push_back("Invalid URL format: " + info.url);
    }

    if (info.weekNumber <= 0) {
        errors.push_back("Week number must be positive");
    }

    return errors;
}

} // namespace validation

} // namespace utec_downloader
