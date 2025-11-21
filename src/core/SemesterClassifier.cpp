#include "utec_downloader/core/SemesterClassifier.hpp"
#include "utec_downloader/utils/Logger.hpp"

#include <sstream>
#include <stdexcept>

namespace utec_downloader {

SemesterClassifier::SemesterClassifier() {
    // Default UTEC semester dates
    // Semester 1: March 10 - July 19
    firstSemester_ = {3, 10, 7, 19};
    // Semester 2: July 28 - December 6
    secondSemester_ = {7, 28, 12, 6};
}

std::string SemesterClassifier::classify(const std::string& dateStr) const {
    auto parsedDate = parseDate(dateStr);
    if (!parsedDate) {
        LOG_WARNING("Invalid date format for classification: " + dateStr);
        return "unknown";
    }

    auto [year, month, day] = *parsedDate;
    Semester semester = getSemester(dateStr);

    std::ostringstream oss;
    oss << year << "-";

    switch (semester) {
        case Semester::First:
            oss << "1";
            break;
        case Semester::Second:
            oss << "2";
            break;
        default:
            oss << "0";
            break;
    }

    return oss.str();
}

SemesterClassifier::Semester SemesterClassifier::getSemester(const std::string& dateStr) const {
    auto parsedDate = parseDate(dateStr);
    if (!parsedDate) {
        return Semester::Unknown;
    }

    auto [year, month, day] = *parsedDate;

    // Check if in first semester range
    if ((month > firstSemester_.startMonth ||
         (month == firstSemester_.startMonth && day >= firstSemester_.startDay)) &&
        (month < firstSemester_.endMonth ||
         (month == firstSemester_.endMonth && day <= firstSemester_.endDay))) {
        return Semester::First;
    }

    // Check if in second semester range
    if ((month > secondSemester_.startMonth ||
         (month == secondSemester_.startMonth && day >= secondSemester_.startDay)) &&
        (month < secondSemester_.endMonth ||
         (month == secondSemester_.endMonth && day <= secondSemester_.endDay))) {
        return Semester::Second;
    }

    return Semester::Unknown;
}

void SemesterClassifier::setFirstSemesterRange(int startMonth, int startDay,
                                                int endMonth, int endDay) {
    firstSemester_ = {startMonth, startDay, endMonth, endDay};
}

void SemesterClassifier::setSecondSemesterRange(int startMonth, int startDay,
                                                 int endMonth, int endDay) {
    secondSemester_ = {startMonth, startDay, endMonth, endDay};
}

int SemesterClassifier::getDayOfYear(const std::string& dateStr) const {
    auto parsedDate = parseDate(dateStr);
    if (!parsedDate) {
        return -1;
    }

    auto [year, month, day] = *parsedDate;
    bool isLeapYear = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);

    return calculateDayOfYear(month, day, isLeapYear);
}

std::optional<std::tuple<int, int, int>> SemesterClassifier::parseDate(
    const std::string& dateStr) const {

    if (dateStr.size() < 10) {
        return std::nullopt;
    }

    try {
        int year = std::stoi(dateStr.substr(0, 4));
        int month = std::stoi(dateStr.substr(5, 2));
        int day = std::stoi(dateStr.substr(8, 2));

        // Basic validation
        if (year < 2000 || year > 2100 ||
            month < 1 || month > 12 ||
            day < 1 || day > 31) {
            return std::nullopt;
        }

        return std::make_tuple(year, month, day);

    } catch (const std::exception&) {
        return std::nullopt;
    }
}

int SemesterClassifier::calculateDayOfYear(int month, int day, bool isLeapYear) const {
    static const int daysBeforeMonth[] = {
        0,    // January
        31,   // February
        59,   // March
        90,   // April
        120,  // May
        151,  // June
        181,  // July
        212,  // August
        243,  // September
        273,  // October
        304,  // November
        334   // December
    };

    int dayOfYear = daysBeforeMonth[month - 1] + day;

    // Adjust for leap year if past February
    if (isLeapYear && month > 2) {
        dayOfYear++;
    }

    return dayOfYear;
}

std::string semesterToString(SemesterClassifier::Semester semester) {
    switch (semester) {
        case SemesterClassifier::Semester::First:
            return "First";
        case SemesterClassifier::Semester::Second:
            return "Second";
        default:
            return "Unknown";
    }
}

} // namespace utec_downloader
