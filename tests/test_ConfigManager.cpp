#include <gtest/gtest.h>
#include "utec_downloader/core/ConfigManager.hpp"

#include <fstream>
#include <filesystem>

using namespace utec_downloader;

class ConfigManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test directory
        testDir_ = std::filesystem::temp_directory_path() / "utec_test_config";
        std::filesystem::create_directories(testDir_);
    }

    void TearDown() override {
        // Clean up test directory
        std::filesystem::remove_all(testDir_);
    }

    void createConfigFile(const std::string& content) {
        configPath_ = testDir_ / "config.txt";
        std::ofstream file(configPath_);
        file << content;
    }

    std::filesystem::path testDir_;
    std::filesystem::path configPath_;
};

TEST_F(ConfigManagerTest, LoadsValidConfigFile) {
    createConfigFile(
        "# Test config\n"
        "download_path=/tmp/downloads\n"
        "max_workers=8\n"
        "quality=best\n"
    );

    ConfigManager manager(configPath_);
    auto config = manager.loadConfig();

    EXPECT_EQ("/tmp/downloads", manager.get("download_path"));
    EXPECT_EQ(8, manager.getInt("max_workers"));
    EXPECT_EQ("best", manager.get("quality"));
}

TEST_F(ConfigManagerTest, HandlesComments) {
    createConfigFile(
        "# This is a comment\n"
        "key1=value1\n"
        "# Another comment\n"
        "key2=value2\n"
    );

    ConfigManager manager(configPath_);
    manager.loadConfig();

    EXPECT_EQ("value1", manager.get("key1"));
    EXPECT_EQ("value2", manager.get("key2"));
}

TEST_F(ConfigManagerTest, HandlesEmptyLines) {
    createConfigFile(
        "key1=value1\n"
        "\n"
        "\n"
        "key2=value2\n"
    );

    ConfigManager manager(configPath_);
    manager.loadConfig();

    EXPECT_EQ("value1", manager.get("key1"));
    EXPECT_EQ("value2", manager.get("key2"));
}

TEST_F(ConfigManagerTest, TrimsWhitespace) {
    createConfigFile(
        "  key1  =  value1  \n"
        "key2=value2\n"
    );

    ConfigManager manager(configPath_);
    manager.loadConfig();

    EXPECT_EQ("value1", manager.get("key1"));
}

TEST_F(ConfigManagerTest, ReturnsDefaultForMissingKey) {
    createConfigFile("key1=value1\n");

    ConfigManager manager(configPath_);
    manager.loadConfig();

    EXPECT_EQ("default", manager.get("missing_key", "default"));
    EXPECT_EQ(42, manager.getInt("missing_int", 42));
}

TEST_F(ConfigManagerTest, GetBoolHandlesVariousFormats) {
    createConfigFile(
        "bool1=true\n"
        "bool2=false\n"
        "bool3=1\n"
        "bool4=0\n"
        "bool5=yes\n"
        "bool6=no\n"
        "bool7=on\n"
        "bool8=off\n"
    );

    ConfigManager manager(configPath_);
    manager.loadConfig();

    EXPECT_TRUE(manager.getBool("bool1"));
    EXPECT_FALSE(manager.getBool("bool2"));
    EXPECT_TRUE(manager.getBool("bool3"));
    EXPECT_FALSE(manager.getBool("bool4"));
    EXPECT_TRUE(manager.getBool("bool5"));
    EXPECT_FALSE(manager.getBool("bool6"));
    EXPECT_TRUE(manager.getBool("bool7"));
    EXPECT_FALSE(manager.getBool("bool8"));
}

TEST_F(ConfigManagerTest, HasKeyReturnsTrueForExistingKey) {
    createConfigFile("key1=value1\n");

    ConfigManager manager(configPath_);
    manager.loadConfig();

    EXPECT_TRUE(manager.hasKey("key1"));
    EXPECT_FALSE(manager.hasKey("nonexistent"));
}

TEST_F(ConfigManagerTest, CreatesDefaultConfigWhenMissing) {
    auto nonExistentPath = testDir_ / "subdir" / "config.txt";
    ConfigManager manager(nonExistentPath);

    // Should not throw
    EXPECT_NO_THROW(manager.loadConfig());

    // File should be created
    EXPECT_TRUE(std::filesystem::exists(nonExistentPath));
}

TEST_F(ConfigManagerTest, GetPlatformReturnsValidString) {
    ConfigManager manager(testDir_ / "config.txt");

    std::string platform = manager.getPlatform();
    EXPECT_TRUE(platform == "linux" || platform == "macos" || platform == "windows");
}

TEST_F(ConfigManagerTest, GetAllReturnsAllConfig) {
    createConfigFile(
        "key1=value1\n"
        "key2=value2\n"
        "key3=value3\n"
    );

    ConfigManager manager(configPath_);
    manager.loadConfig();

    auto all = manager.getAll();
    EXPECT_GE(all.size(), 3u);
}
