#include <gtest/gtest.h>
#include "utec_downloader/core/DataParser.hpp"

#include <fstream>
#include <filesystem>

using namespace utec_downloader;

class DataParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        testDir_ = std::filesystem::temp_directory_path() / "utec_test_parser";
        std::filesystem::create_directories(testDir_);
    }

    void TearDown() override {
        std::filesystem::remove_all(testDir_);
    }

    std::filesystem::path createJsonFile(const std::string& content,
                                          const std::string& filename = "test.json") {
        auto path = testDir_ / filename;
        std::ofstream file(path);
        file << content;
        return path;
    }

    std::filesystem::path testDir_;
};

TEST_F(DataParserTest, ParsesSingleObjectJson) {
    auto path = createJsonFile(R"({
        "subject": "Matemáticas",
        "fecha": "2024-04-15",
        "url": "https://example.com/video1",
        "weekNumber": 5,
        "seccion": "TEORÍA",
        "modalidad": "PRESENCIAL"
    })");

    DataParser parser;
    auto classes = parser.parseFile(path);

    ASSERT_EQ(1u, classes.size());
    EXPECT_EQ("Matemáticas", classes[0].subject);
    EXPECT_EQ("2024-04-15", classes[0].fecha);
    EXPECT_EQ("https://example.com/video1", classes[0].url);
    EXPECT_EQ(5, classes[0].weekNumber);
}

TEST_F(DataParserTest, ParsesArrayJson) {
    auto path = createJsonFile(R"([
        {
            "subject": "Física",
            "fecha": "2024-04-16",
            "url": "https://example.com/video1",
            "weekNumber": 5
        },
        {
            "subject": "Química",
            "fecha": "2024-04-17",
            "url": "https://example.com/video2",
            "weekNumber": 5
        }
    ])");

    DataParser parser;
    auto classes = parser.parseFile(path);

    ASSERT_EQ(2u, classes.size());
    EXPECT_EQ("Física", classes[0].subject);
    EXPECT_EQ("Química", classes[1].subject);
}

TEST_F(DataParserTest, HandlesAlternativeFieldNames) {
    auto path = createJsonFile(R"({
        "materia": "Algebra",
        "date": "2024-04-15",
        "link": "https://example.com/video",
        "semana": 3
    })");

    DataParser parser;
    auto classes = parser.parseFile(path);

    ASSERT_EQ(1u, classes.size());
    EXPECT_EQ("Algebra", classes[0].subject);
    EXPECT_EQ("2024-04-15", classes[0].fecha);
    EXPECT_EQ("https://example.com/video", classes[0].url);
    EXPECT_EQ(3, classes[0].weekNumber);
}

TEST_F(DataParserTest, SkipsInvalidEntries) {
    auto path = createJsonFile(R"([
        {
            "subject": "Valid",
            "fecha": "2024-04-15",
            "url": "https://example.com/video",
            "weekNumber": 1
        },
        {
            "subject": "",
            "fecha": "",
            "url": "",
            "weekNumber": 0
        }
    ])");

    DataParser parser;
    auto classes = parser.parseFile(path);

    EXPECT_EQ(1u, classes.size());
    EXPECT_EQ("Valid", classes[0].subject);
}

TEST_F(DataParserTest, ReturnsEmptyForNonexistentFile) {
    DataParser parser;
    auto classes = parser.parseFile("/nonexistent/file.json");

    EXPECT_TRUE(classes.empty());
}

TEST_F(DataParserTest, ReturnsEmptyForInvalidJson) {
    auto path = createJsonFile("this is not valid json {{{");

    DataParser parser;
    auto classes = parser.parseFile(path);

    EXPECT_TRUE(classes.empty());
}

TEST_F(DataParserTest, RemovesDuplicatesByUrl) {
    DataParser parser;

    std::vector<ClassInfo> classes;
    ClassInfo info1;
    info1.subject = "Math";
    info1.fecha = "2024-04-15";
    info1.url = "https://example.com/video1";
    info1.weekNumber = 1;

    ClassInfo info2;
    info2.subject = "Math";
    info2.fecha = "2024-04-16";
    info2.url = "https://example.com/video1"; // Same URL
    info2.weekNumber = 2;

    ClassInfo info3;
    info3.subject = "Physics";
    info3.fecha = "2024-04-17";
    info3.url = "https://example.com/video2"; // Different URL
    info3.weekNumber = 1;

    classes.push_back(info1);
    classes.push_back(info2);
    classes.push_back(info3);

    auto unique = parser.removeDuplicates(classes);

    EXPECT_EQ(2u, unique.size());
}

TEST_F(DataParserTest, ParsesMultipleFiles) {
    createJsonFile(R"([{"subject": "A", "fecha": "2024-01-01", "url": "https://a.com", "weekNumber": 1}])", "file1.json");
    createJsonFile(R"([{"subject": "B", "fecha": "2024-01-02", "url": "https://b.com", "weekNumber": 1}])", "file2.json");

    DataParser parser;
    std::vector<std::filesystem::path> files = {
        testDir_ / "file1.json",
        testDir_ / "file2.json"
    };

    auto classes = parser.parseMultipleFiles(files);

    EXPECT_EQ(2u, classes.size());
}

TEST_F(DataParserTest, FindJsonFilesReturnsOnlyJsonFiles) {
    createJsonFile("{}", "file1.json");
    createJsonFile("{}", "file2.json");

    // Create a non-JSON file
    std::ofstream(testDir_ / "file.txt") << "not json";

    auto files = findJsonFiles(testDir_);

    EXPECT_EQ(2u, files.size());
    for (const auto& file : files) {
        EXPECT_EQ(".json", file.extension());
    }
}

TEST_F(DataParserTest, FindJsonFilesReturnsEmptyForNonexistentDir) {
    auto files = findJsonFiles("/nonexistent/directory");
    EXPECT_TRUE(files.empty());
}

TEST_F(DataParserTest, ValidateClassDataValidatesCorrectly) {
    ClassInfo valid;
    valid.subject = "Test";
    valid.fecha = "2024-01-01";
    valid.url = "https://example.com";
    valid.weekNumber = 1;

    EXPECT_TRUE(DataParser::validateClassData(valid));

    ClassInfo invalid;
    EXPECT_FALSE(DataParser::validateClassData(invalid));
}

// Tests for UTEC Extractor JSON formats

TEST_F(DataParserTest, NormalizesDateFromDDMMYYYYFormat) {
    auto path = createJsonFile(R"({
        "subject": "Matemáticas",
        "fecha": "15/04/2024",
        "url": "https://example.com/video1",
        "weekNumber": 5
    })");

    DataParser parser;
    auto classes = parser.parseFile(path);

    ASSERT_EQ(1u, classes.size());
    // Date should be normalized to YYYY-MM-DD
    EXPECT_EQ("2024-04-15", classes[0].fecha);
}

TEST_F(DataParserTest, PreservesISODateFormat) {
    auto path = createJsonFile(R"({
        "subject": "Física",
        "fecha": "2024-04-15",
        "url": "https://example.com/video1",
        "weekNumber": 5
    })");

    DataParser parser;
    auto classes = parser.parseFile(path);

    ASSERT_EQ(1u, classes.size());
    EXPECT_EQ("2024-04-15", classes[0].fecha);
}

TEST_F(DataParserTest, ParsesAllWeeksWithMetadataFormat) {
    auto path = createJsonFile(R"({
        "extractionDate": "2024-04-15",
        "periodo": "2024-1",
        "totalRecordings": 3,
        "totalWeeks": 2,
        "weeks": {
            "1": [
                {"subject": "Math", "fecha": "15/04/2024", "url": "https://a.com", "weekNumber": 1}
            ],
            "2": [
                {"subject": "Physics", "fecha": "16/04/2024", "url": "https://b.com", "weekNumber": 2},
                {"subject": "Chemistry", "fecha": "17/04/2024", "url": "https://c.com", "weekNumber": 2}
            ]
        }
    })");

    DataParser parser;
    auto classes = parser.parseFile(path);

    ASSERT_EQ(3u, classes.size());
    EXPECT_EQ("Math", classes[0].subject);
    EXPECT_EQ(1, classes[0].weekNumber);
    EXPECT_EQ("2024-04-15", classes[0].fecha);  // Date normalized
}

TEST_F(DataParserTest, ParsesAllWeeksWithoutMetadataFormat) {
    auto path = createJsonFile(R"({
        "1": [
            {"subject": "Math", "fecha": "15/04/2024", "url": "https://a.com"}
        ],
        "2": [
            {"subject": "Physics", "fecha": "16/04/2024", "url": "https://b.com"}
        ]
    })");

    DataParser parser;
    auto classes = parser.parseFile(path);

    ASSERT_EQ(2u, classes.size());
    // Week number should be set from the key
    EXPECT_EQ(1, classes[0].weekNumber);
    EXPECT_EQ(2, classes[1].weekNumber);
}

TEST_F(DataParserTest, ParsesNewUTECExtractorFields) {
    auto path = createJsonFile(R"({
        "subject": "Cálculo I - MA101",
        "fecha": "15/04/2024",
        "horaInicio": "09:00",
        "url": "https://zoom.us/rec/123",
        "weekNumber": 5,
        "seccion": "AB1",
        "modalidad": "Virtual",
        "docente": "Dr. Juan Pérez",
        "tipo": "Teoría",
        "estado": "Grabado",
        "title": "Clase 5 - Derivadas",
        "timestamp": 1713168000000,
        "buttonId": "ver_12345"
    })");

    DataParser parser;
    auto classes = parser.parseFile(path);

    ASSERT_EQ(1u, classes.size());
    EXPECT_EQ("Cálculo I - MA101", classes[0].subject);
    EXPECT_EQ("AB1", classes[0].seccion);
    EXPECT_EQ("Dr. Juan Pérez", classes[0].docente);
    EXPECT_EQ("Teoría", classes[0].tipo);
    EXPECT_EQ("Grabado", classes[0].estado);
    EXPECT_EQ("Clase 5 - Derivadas", classes[0].title);
    EXPECT_EQ(1713168000000, classes[0].timestamp);
    EXPECT_EQ("ver_12345", classes[0].buttonId);
}

TEST_F(DataParserTest, HandlesEmptyWeeksInMetadataFormat) {
    auto path = createJsonFile(R"({
        "weeks": {
            "1": [],
            "2": [
                {"subject": "Physics", "fecha": "16/04/2024", "url": "https://b.com", "weekNumber": 2}
            ],
            "3": []
        }
    })");

    DataParser parser;
    auto classes = parser.parseFile(path);

    ASSERT_EQ(1u, classes.size());
    EXPECT_EQ("Physics", classes[0].subject);
}
