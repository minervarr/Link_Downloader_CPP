#include <gtest/gtest.h>
#include "utec_downloader/core/FileOrganizer.hpp"

#include <filesystem>
#include <fstream>

using namespace utec_downloader;

class FileOrganizerTest : public ::testing::Test {
protected:
    void SetUp() override {
        testDir_ = std::filesystem::temp_directory_path() / "utec_test_organizer";
        std::filesystem::create_directories(testDir_);
        organizer_ = std::make_unique<FileOrganizer>(testDir_);
    }

    void TearDown() override {
        std::filesystem::remove_all(testDir_);
    }

    ClassInfo createTestClassInfo() {
        ClassInfo info;
        info.subject = "Matematicas I";
        info.fecha = "2024-04-15";
        info.horaInicio = "09:00";
        info.url = "https://example.com/video";
        info.weekNumber = 5;
        info.seccion = "TEORIA";
        info.modalidad = "PRESENCIAL";
        return info;
    }

    std::filesystem::path testDir_;
    std::unique_ptr<FileOrganizer> organizer_;
};

TEST_F(FileOrganizerTest, OrganizeClassCreatesCorrectPath) {
    auto info = createTestClassInfo();

    auto path = organizer_->organizeClass(info, "2024-1");

    // Should create: basePath/semester/Semana##/filename.mp4
    EXPECT_TRUE(path.string().find("2024-1") != std::string::npos);
    EXPECT_TRUE(path.string().find("Semana05") != std::string::npos);
    EXPECT_TRUE(path.extension() == ".mp4");
}

TEST_F(FileOrganizerTest, CreateDirectoryStructureCreatesDirectories) {
    auto info = createTestClassInfo();

    auto dirPath = organizer_->createDirectoryStructure(info, "2024-1");

    EXPECT_TRUE(std::filesystem::exists(dirPath));
    EXPECT_TRUE(std::filesystem::is_directory(dirPath));
    EXPECT_TRUE(dirPath.string().find("Semana05") != std::string::npos);
}

TEST_F(FileOrganizerTest, FileExistsReturnsFalseForNewFiles) {
    auto info = createTestClassInfo();

    EXPECT_FALSE(organizer_->fileExists(info, "2024-1"));
}

TEST_F(FileOrganizerTest, FileExistsReturnsTrueForExistingFiles) {
    auto info = createTestClassInfo();

    // Create the file
    auto path = organizer_->organizeClass(info, "2024-1");
    std::filesystem::create_directories(path.parent_path());
    std::ofstream(path) << "test content";

    EXPECT_TRUE(organizer_->fileExists(info, "2024-1"));
}

TEST_F(FileOrganizerTest, CleansSubjectNameWithSpecialCharacters) {
    auto info = createTestClassInfo();
    info.subject = "Test: Subject/With\\Special<Characters>";

    auto path = organizer_->organizeClass(info, "2024-1");

    // Path should not contain invalid characters
    std::string pathStr = path.string();
    EXPECT_TRUE(pathStr.find('<') == std::string::npos);
    EXPECT_TRUE(pathStr.find('>') == std::string::npos);
}

TEST_F(FileOrganizerTest, DirectoryContainsWeekNumber) {
    auto info = createTestClassInfo();
    info.weekNumber = 14;

    auto path = organizer_->organizeClass(info, "2024-1");
    std::string pathStr = path.string();

    // Week number should be in directory path (Semana14)
    EXPECT_TRUE(pathStr.find("Semana14") != std::string::npos);
}

TEST_F(FileOrganizerTest, IncludesDateInFilename) {
    auto info = createTestClassInfo();

    auto path = organizer_->organizeClass(info, "2024-1");
    std::string filename = path.filename().string();

    EXPECT_TRUE(filename.find("2024-04-15") != std::string::npos);
}

TEST_F(FileOrganizerTest, IncludesTimeInFilename) {
    auto info = createTestClassInfo();
    info.horaInicio = "14:30";

    auto path = organizer_->organizeClass(info, "2024-1");
    std::string filename = path.filename().string();

    // Time should be in filename with dash instead of colon
    EXPECT_TRUE(filename.find("14-30") != std::string::npos);
}

TEST_F(FileOrganizerTest, IncludesSectionInFilename) {
    auto info = createTestClassInfo();
    info.seccion = "LABORATORIO - 2.01";

    auto path = organizer_->organizeClass(info, "2024-1");
    std::string filename = path.filename().string();

    // Should contain section
    EXPECT_TRUE(filename.find("LABORATORIO") != std::string::npos);
}

TEST_F(FileOrganizerTest, ReplacesSpacesWithUnderscores) {
    auto info = createTestClassInfo();
    info.subject = "Circuitos Digitales - EL2013";

    auto path = organizer_->organizeClass(info, "2024-1");
    std::string filename = path.filename().string();

    // Subject in filename should have underscores instead of spaces
    EXPECT_TRUE(filename.find("Circuitos_Digitales") != std::string::npos);
}

TEST_F(FileOrganizerTest, GetBasePathReturnsCorrectPath) {
    EXPECT_EQ(testDir_, organizer_->getBasePath());
}

TEST_F(FileOrganizerTest, SetBasePathChangesPath) {
    auto newPath = testDir_ / "new_base";
    organizer_->setBasePath(newPath);

    EXPECT_EQ(newPath, organizer_->getBasePath());
}

TEST_F(FileOrganizerTest, HandlesLongSubjectNames) {
    auto info = createTestClassInfo();
    // Create a very long subject name
    info.subject = std::string(200, 'A');

    auto path = organizer_->organizeClass(info, "2024-1");

    // Filename should be truncated to a reasonable length
    EXPECT_LT(path.filename().string().length(), 200u);
}

TEST_F(FileOrganizerTest, CreatesBaseDirectoryIfNotExists) {
    auto newBasePath = testDir_ / "new" / "nested" / "directory";

    // Should not throw
    EXPECT_NO_THROW({
        FileOrganizer newOrganizer(newBasePath);
    });

    EXPECT_TRUE(std::filesystem::exists(newBasePath));
}

TEST_F(FileOrganizerTest, HandlesEmptySeccionAndModalidad) {
    auto info = createTestClassInfo();
    info.seccion = "";
    info.modalidad = "";

    // Should not throw
    auto path = organizer_->organizeClass(info, "2024-1");
    EXPECT_FALSE(path.empty());
}

TEST_F(FileOrganizerTest, HandlesEmptyHoraInicio) {
    auto info = createTestClassInfo();
    info.horaInicio = "";

    auto path = organizer_->organizeClass(info, "2024-1");
    std::string filename = path.filename().string();

    // Should still generate valid filename
    EXPECT_TRUE(filename.find("2024-04-15") != std::string::npos);
    EXPECT_TRUE(filename.find(".mp4") != std::string::npos);
}

TEST_F(FileOrganizerTest, FilenameFormatIsCorrect) {
    auto info = createTestClassInfo();
    info.subject = "Circuitos Digitales - EL2013";
    info.fecha = "2025-11-19";
    info.horaInicio = "08:00";
    info.seccion = "TEORIA - 2";
    info.weekNumber = 14;

    auto path = organizer_->organizeClass(info, "2025-2");
    std::string filename = path.filename().string();

    // Expected format: fecha_horaInicio_subject_seccion.mp4
    // 2025-11-19_08-00_Circuitos_Digitales_-_EL2013_TEORIA---2.mp4
    EXPECT_TRUE(filename.find("2025-11-19") != std::string::npos);
    EXPECT_TRUE(filename.find("08-00") != std::string::npos);
    EXPECT_TRUE(filename.find("Circuitos") != std::string::npos);
    EXPECT_TRUE(filename.find("TEORIA") != std::string::npos);
    EXPECT_EQ(".mp4", path.extension().string());
}
