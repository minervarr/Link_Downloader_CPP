#ifndef UTEC_DOWNLOADER_CLASSINFO_HPP
#define UTEC_DOWNLOADER_CLASSINFO_HPP

#include <string>
#include <optional>
#include <vector>
#include <cstdint>

namespace utec_downloader {

/**
 * @brief Represents metadata for a class recording
 *
 * Contains all necessary information about a class session including
 * subject, date, URL, and classification details.
 * Aligned with UTEC Extractor JSON structure specification.
 */
struct ClassInfo {
    // Core fields
    std::string subject;      ///< Course name with code (e.g., "Matemáticas I - MA101")
    std::string fecha;        ///< Date in YYYY-MM-DD format (internally normalized)
    std::string horaInicio;   ///< Start time in HH:MM format (e.g., "09:00")
    std::string url;          ///< Complete Zoom recording link
    int weekNumber{0};        ///< Academic week identifier (1-20)
    std::string seccion;      ///< Class section (e.g., "AB1", "CD2")
    std::string modalidad;    ///< Delivery method: "Virtual", "Presencial", "Híbrido"

    // New fields from UTEC Extractor
    std::string docente;      ///< Instructor name
    std::string tipo;         ///< Class category: "Teoría", "Práctica", "Laboratorio"
    std::string estado;       ///< Status: "Grabado", "Pendiente"
    std::string title;        ///< Recording name from Zoom
    int64_t timestamp{0};     ///< Milliseconds since Unix epoch
    std::string buttonId;     ///< Internal identifier (ver_XXXXX format)

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
