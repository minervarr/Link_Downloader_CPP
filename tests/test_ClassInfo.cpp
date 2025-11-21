#include <gtest/gtest.h>
#include "utec_downloader/core/ClassInfo.hpp"

using namespace utec_downloader;

class ClassInfoTest : public ::testing::Test {
protected:
    ClassInfo createValidClassInfo() {
        ClassInfo info;
        info.subject = "Matemáticas I";
        info.fecha = "2024-04-15";
        info.url = "https://zoom.us/rec/12345";
        info.weekNumber = 5;
        info.seccion = "TEORÍA";
        info.modalidad = "PRESENCIAL";
        return info;
    }
};

TEST_F(ClassInfoTest, ValidClassInfoIsValid) {
    auto info = createValidClassInfo();
    EXPECT_TRUE(info.isValid());
}

TEST_F(ClassInfoTest, EmptySubjectIsInvalid) {
    auto info = createValidClassInfo();
    info.subject = "";
    EXPECT_FALSE(info.isValid());
}

TEST_F(ClassInfoTest, EmptyUrlIsInvalid) {
    auto info = createValidClassInfo();
    info.url = "";
    EXPECT_FALSE(info.isValid());
}

TEST_F(ClassInfoTest, EmptyFechaIsInvalid) {
    auto info = createValidClassInfo();
    info.fecha = "";
    EXPECT_FALSE(info.isValid());
}

TEST_F(ClassInfoTest, ZeroWeekNumberIsInvalid) {
    auto info = createValidClassInfo();
    info.weekNumber = 0;
    EXPECT_FALSE(info.isValid());
}

TEST_F(ClassInfoTest, NegativeWeekNumberIsInvalid) {
    auto info = createValidClassInfo();
    info.weekNumber = -1;
    EXPECT_FALSE(info.isValid());
}

TEST_F(ClassInfoTest, GetYearExtractsCorrectYear) {
    auto info = createValidClassInfo();
    EXPECT_EQ("2024", info.getYear());
}

TEST_F(ClassInfoTest, GetYearReturnsEmptyForShortDate) {
    ClassInfo info;
    info.fecha = "20";
    EXPECT_EQ("", info.getYear());
}

TEST_F(ClassInfoTest, GetUniqueIdGeneratesConsistentId) {
    auto info = createValidClassInfo();
    auto id1 = info.getUniqueId();
    auto id2 = info.getUniqueId();
    EXPECT_EQ(id1, id2);
}

TEST_F(ClassInfoTest, DifferentInfoHasDifferentUniqueId) {
    auto info1 = createValidClassInfo();
    auto info2 = createValidClassInfo();
    info2.weekNumber = 6;

    EXPECT_NE(info1.getUniqueId(), info2.getUniqueId());
}

TEST_F(ClassInfoTest, EqualityOperatorWorksCorrectly) {
    auto info1 = createValidClassInfo();
    auto info2 = createValidClassInfo();
    EXPECT_EQ(info1, info2);

    info2.subject = "Física";
    EXPECT_NE(info1, info2);
}

// Validation namespace tests
namespace {

TEST(ValidationTest, ValidDateFormats) {
    EXPECT_TRUE(validation::isValidDate("2024-01-15"));
    EXPECT_TRUE(validation::isValidDate("2024-12-31"));
    EXPECT_TRUE(validation::isValidDate("2000-06-01"));
}

TEST(ValidationTest, InvalidDateFormats) {
    EXPECT_FALSE(validation::isValidDate(""));
    EXPECT_FALSE(validation::isValidDate("2024/01/15"));
    EXPECT_FALSE(validation::isValidDate("15-01-2024"));
    EXPECT_FALSE(validation::isValidDate("invalid"));
    EXPECT_FALSE(validation::isValidDate("2024-13-01")); // Invalid month
    EXPECT_FALSE(validation::isValidDate("2024-01-32")); // Invalid day
}

TEST(ValidationTest, ValidUrls) {
    EXPECT_TRUE(validation::isValidUrl("https://zoom.us/rec/12345"));
    EXPECT_TRUE(validation::isValidUrl("http://example.com/video.mp4"));
    EXPECT_TRUE(validation::isValidUrl("https://www.youtube.com/watch?v=abc123"));
}

TEST(ValidationTest, InvalidUrls) {
    EXPECT_FALSE(validation::isValidUrl(""));
    EXPECT_FALSE(validation::isValidUrl("not-a-url"));
    EXPECT_FALSE(validation::isValidUrl("ftp://invalid-scheme.com"));
}

TEST(ValidationTest, ValidateClassInfoReturnsNoErrorsForValidData) {
    ClassInfo info;
    info.subject = "Test Subject";
    info.fecha = "2024-05-15";
    info.url = "https://example.com/video";
    info.weekNumber = 1;

    auto errors = validation::validateClassInfo(info);
    EXPECT_TRUE(errors.empty());
}

TEST(ValidationTest, ValidateClassInfoReturnsErrorsForInvalidData) {
    ClassInfo info; // All fields empty/zero

    auto errors = validation::validateClassInfo(info);
    EXPECT_FALSE(errors.empty());
    EXPECT_GE(errors.size(), 3u); // At least subject, fecha, url errors
}

} // anonymous namespace
