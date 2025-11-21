#ifndef UTEC_DOWNLOADER_SEMESTERCLASSIFIER_HPP
#define UTEC_DOWNLOADER_SEMESTERCLASSIFIER_HPP

#include <string>
#include <chrono>
#include <optional>

namespace utec_downloader {

/**
 * @brief Classifies classes into semesters based on dates
 *
 * Uses predefined date ranges to determine whether a class
 * belongs to Semester 1 or Semester 2 of an academic year.
 */
class SemesterClassifier {
public:
    /**
     * @brief Semester identifiers
     */
    enum class Semester {
        First,   ///< First semester (March - July)
        Second,  ///< Second semester (August - December)
        Unknown  ///< Date outside semester ranges
    };

    /**
     * @brief Semester date range configuration
     */
    struct SemesterRange {
        int startMonth;
        int startDay;
        int endMonth;
        int endDay;
    };

    SemesterClassifier();

    /**
     * @brief Classifies a date into a semester
     * @param dateStr Date in YYYY-MM-DD format
     * @return Semester identifier string (e.g., "2024-1" or "2024-2")
     */
    [[nodiscard]] std::string classify(const std::string& dateStr) const;

    /**
     * @brief Gets the semester enum for a date
     * @param dateStr Date in YYYY-MM-DD format
     * @return Semester enum value
     */
    [[nodiscard]] Semester getSemester(const std::string& dateStr) const;

    /**
     * @brief Configures custom semester date ranges
     */
    void setFirstSemesterRange(int startMonth, int startDay, int endMonth, int endDay);
    void setSecondSemesterRange(int startMonth, int startDay, int endMonth, int endDay);

private:
    [[nodiscard]] int getDayOfYear(const std::string& dateStr) const;
    [[nodiscard]] std::optional<std::tuple<int, int, int>> parseDate(
        const std::string& dateStr) const;
    [[nodiscard]] int calculateDayOfYear(int month, int day, bool isLeapYear) const;

    SemesterRange firstSemester_;
    SemesterRange secondSemester_;
};

/**
 * @brief Converts Semester enum to string
 */
[[nodiscard]] std::string semesterToString(SemesterClassifier::Semester semester);

} // namespace utec_downloader

#endif // UTEC_DOWNLOADER_SEMESTERCLASSIFIER_HPP
