#ifndef UTEC_DOWNLOADER_CLASSINFO_HPP
#define UTEC_DOWNLOADER_CLASSINFO_HPP

#include <string>
#include <optional>
#include <vector>

namespace utec_downloader {

/**
 * @brief Represents metadata for a class recording
 *
 * Contains all necessary information about a class session including
 * subject, date, URL, and classification details.
 */
struct ClassInfo {
    std::string subject;      ///< Course name (e.g., "Matemáticas I")
    std::string fecha;        ///< Date in YYYY-MM-DD format
    std::string url;          ///< Video URL
    int weekNumber{0};        ///< Week number in the semester
    std::string seccion;      ///< Section type: "TEORÍA", "LABORATORIO", etc.
    std::string modalidad;    ///< Delivery mode: "PRESENCIAL", "VIRTUAL"

    /**
     * @brief Validates that all required fields are properly set
     * @return true if the ClassInfo has valid data
     */
    [[nodiscard]] bool isValid() const noexcept;

    /**
     * @brief Extracts the year from the fecha field
     * @return Year as string, or empty string if fecha is invalid
     */
    [[nodiscard]] std::string getYear() const noexcept;

    /**
     * @brief Generates a unique identifier for this class
     * @return Unique string identifier based on subject, date, and section
     */
    [[nodiscard]] std::string getUniqueId() const;

    /**
     * @brief Equality comparison
     */
    bool operator==(const ClassInfo& other) const noexcept;
    bool operator!=(const ClassInfo& other) const noexcept;
};

/**
 * @brief Result type for operations that can fail
 */
template<typename T>
struct Result {
    std::optional<T> value;
    std::string error;

    [[nodiscard]] bool isOk() const noexcept { return value.has_value(); }
    [[nodiscard]] bool isError() const noexcept { return !value.has_value(); }

    static Result<T> ok(T val) { return {std::move(val), ""}; }
    static Result<T> err(std::string msg) { return {std::nullopt, std::move(msg)}; }
};

/**
 * @brief Collection of validation functions for ClassInfo
 */
namespace validation {
    [[nodiscard]] bool isValidDate(const std::string& date);
    [[nodiscard]] bool isValidUrl(const std::string& url);
    [[nodiscard]] std::vector<std::string> validateClassInfo(const ClassInfo& info);
}

} // namespace utec_downloader

#endif // UTEC_DOWNLOADER_CLASSINFO_HPP
