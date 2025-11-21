#include <gtest/gtest.h>
#include "utec_downloader/core/SemesterClassifier.hpp"

using namespace utec_downloader;

class SemesterClassifierTest : public ::testing::Test {
protected:
    SemesterClassifier classifier_;
};

TEST_F(SemesterClassifierTest, ClassifiesFirstSemesterDates) {
    // Semester 1: March 10 - July 19
    EXPECT_EQ("2024-1", classifier_.classify("2024-03-15"));
    EXPECT_EQ("2024-1", classifier_.classify("2024-04-20"));
    EXPECT_EQ("2024-1", classifier_.classify("2024-05-01"));
    EXPECT_EQ("2024-1", classifier_.classify("2024-06-30"));
    EXPECT_EQ("2024-1", classifier_.classify("2024-07-10"));
}

TEST_F(SemesterClassifierTest, ClassifiesSecondSemesterDates) {
    // Semester 2: July 28 - December 6
    EXPECT_EQ("2024-2", classifier_.classify("2024-08-15"));
    EXPECT_EQ("2024-2", classifier_.classify("2024-09-20"));
    EXPECT_EQ("2024-2", classifier_.classify("2024-10-01"));
    EXPECT_EQ("2024-2", classifier_.classify("2024-11-15"));
    EXPECT_EQ("2024-2", classifier_.classify("2024-12-01"));
}

TEST_F(SemesterClassifierTest, ClassifiesBoundaryDates) {
    // First semester boundaries
    EXPECT_EQ("2024-1", classifier_.classify("2024-03-10")); // Start
    EXPECT_EQ("2024-1", classifier_.classify("2024-07-19")); // End

    // Second semester boundaries
    EXPECT_EQ("2024-2", classifier_.classify("2024-07-28")); // Start
    EXPECT_EQ("2024-2", classifier_.classify("2024-12-06")); // End
}

TEST_F(SemesterClassifierTest, ReturnsUnknownForOutOfRangeDates) {
    // Between semesters
    EXPECT_EQ("2024-0", classifier_.classify("2024-07-20"));
    EXPECT_EQ("2024-0", classifier_.classify("2024-07-25"));

    // After second semester
    EXPECT_EQ("2024-0", classifier_.classify("2024-12-20"));

    // Before first semester
    EXPECT_EQ("2024-0", classifier_.classify("2024-01-15"));
    EXPECT_EQ("2024-0", classifier_.classify("2024-02-28"));
}

TEST_F(SemesterClassifierTest, ReturnsUnknownForInvalidDates) {
    EXPECT_EQ("unknown", classifier_.classify("invalid"));
    EXPECT_EQ("unknown", classifier_.classify(""));
    EXPECT_EQ("unknown", classifier_.classify("2024"));
    EXPECT_EQ("unknown", classifier_.classify("not-a-date"));
}

TEST_F(SemesterClassifierTest, GetSemesterReturnsCorrectEnum) {
    EXPECT_EQ(SemesterClassifier::Semester::First,
              classifier_.getSemester("2024-05-15"));
    EXPECT_EQ(SemesterClassifier::Semester::Second,
              classifier_.getSemester("2024-10-15"));
    EXPECT_EQ(SemesterClassifier::Semester::Unknown,
              classifier_.getSemester("2024-01-01"));
    EXPECT_EQ(SemesterClassifier::Semester::Unknown,
              classifier_.getSemester("invalid"));
}

TEST_F(SemesterClassifierTest, CustomSemesterRangesWork) {
    SemesterClassifier customClassifier;

    // Set custom ranges
    customClassifier.setFirstSemesterRange(2, 1, 6, 30); // Feb 1 - June 30
    customClassifier.setSecondSemesterRange(8, 1, 12, 31); // Aug 1 - Dec 31

    EXPECT_EQ(SemesterClassifier::Semester::First,
              customClassifier.getSemester("2024-03-15"));
    EXPECT_EQ(SemesterClassifier::Semester::Second,
              customClassifier.getSemester("2024-09-15"));
    EXPECT_EQ(SemesterClassifier::Semester::Unknown,
              customClassifier.getSemester("2024-07-15"));
}

TEST_F(SemesterClassifierTest, HandlesLeapYears) {
    // Feb 29 in leap year
    EXPECT_EQ("2024-0", classifier_.classify("2024-02-29"));

    // These should still work correctly
    EXPECT_EQ("2024-1", classifier_.classify("2024-03-15"));
    EXPECT_EQ("2024-2", classifier_.classify("2024-09-15"));
}

TEST_F(SemesterClassifierTest, PreservesYearInOutput) {
    EXPECT_EQ("2020-1", classifier_.classify("2020-05-15"));
    EXPECT_EQ("2021-2", classifier_.classify("2021-09-15"));
    EXPECT_EQ("2025-1", classifier_.classify("2025-04-10"));
}

TEST_F(SemesterClassifierTest, SemesterToStringWorks) {
    EXPECT_EQ("First", semesterToString(SemesterClassifier::Semester::First));
    EXPECT_EQ("Second", semesterToString(SemesterClassifier::Semester::Second));
    EXPECT_EQ("Unknown", semesterToString(SemesterClassifier::Semester::Unknown));
}
