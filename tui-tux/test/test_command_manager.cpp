#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <agency_request_wrapper.hpp>
#include <agency_manager.hpp>
#include <bookmark_manager.hpp>
#include <command_manager.hpp>
#include <fstream>

using namespace testing;

class MockAgencyManager final : public AgencyManager {
public:
    explicit MockAgencyManager(AgencyRequestWrapper *agency_request_wrapper) : AgencyManager(agency_request_wrapper) {}

    MOCK_METHOD(void, set_agent_name, (const std::string &agent_name), (override));
};

class MockBookmarkManager final : public BookmarkManager {
public:
    explicit MockBookmarkManager(AgencyManager* agency_mgr) : BookmarkManager(agency_mgr) {}

    MOCK_METHOD(void, bookmark, (int index, const std::string &alias), (override));
    MOCK_METHOD(void, list_bookmarks, (), (const, override));
    MOCK_METHOD(void, remove_bookmark, (const std::string &alias), (override));
};

class MockCommandManager final : public CommandManager {
public:
    explicit MockCommandManager(BookmarkManager *bookmark_manager) : CommandManager(bookmark_manager) {}

    MOCK_METHOD(int, read_from_file, (std::string &filepath, std::string &output), (override));
};

class CommandManagerTest : public Test {
public:
    AgencyRequestWrapper request_wrapper;
    AgencyManager agency_manager;
    BookmarkManager bookmark_manager;
    CommandManager command_manager;

    MockAgencyManager mock_agency_manager;
    MockBookmarkManager mock_bookmark_manager;
    MockCommandManager mock_command_manager;

    std::stringstream output_stream;
    std::stringstream error_stream;

    std::streambuf *original_cout{};
    std::streambuf *original_cerr{};

    CommandManagerTest() : agency_manager(&request_wrapper), bookmark_manager(&agency_manager), command_manager(&bookmark_manager), mock_agency_manager(&request_wrapper), mock_bookmark_manager(&mock_agency_manager), mock_command_manager(&mock_bookmark_manager) {}

    void SetUp() override {
        // redirect std::cout to output_stream
        original_cout = std::cout.rdbuf();
        original_cerr = std::cerr.rdbuf();

        std::cout.rdbuf(output_stream.rdbuf());
        std::cerr.rdbuf(error_stream.rdbuf());

        mock_bookmark_manager.bookmarks.clear();
        mock_bookmark_manager.bookmarks["alias1"] = {"query1", "result1"};
        agency_manager.session_history.clear();
        mock_agency_manager.session_history.clear();
    }

    void TearDown() override {
        // reset the std::cout buffer back to default
        std::cout.rdbuf(original_cout);
        std::cerr.rdbuf(original_cerr);
    }
};

// Test case: Correctly parses valid bookmark command with index and alias.
TEST_F(CommandManagerTest, Bookmark_ValidWithIndexAndAlias) {
    EXPECT_CALL(mock_bookmark_manager, bookmark(2, "alias"));

    std::string command = "bookmark 2 alias";
    mock_command_manager.run_command(command);
}

// Test case: Correctly parses valid bookmark command with alias only.
TEST_F(CommandManagerTest, Bookmark_ValidWithAliasOnly) {
    EXPECT_CALL(mock_bookmark_manager, bookmark(1, "alias"));

    std::string command = "bookmark alias";
    mock_command_manager.run_command(command);
}

// Test case: Invalid command
TEST_F(CommandManagerTest, InvalidCommand) {
    std::string command = "invalid_command alias";
    mock_command_manager.run_command(command);

    EXPECT_TRUE(error_stream.str().find("Error") != std::string::npos);
}

// Test case: Correctly handles list flag.
TEST_F(CommandManagerTest, Bookmark_HandlesListFlag) {
    EXPECT_CALL(mock_bookmark_manager, list_bookmarks())
        .Times(2);

    std::string command1 = "bookmark --list";
    std::string command2 = "bookmark -l";
    mock_command_manager.run_command(command1);
    mock_command_manager.run_command(command2);
}

// Test case: Correctly handles remove flag.
TEST_F(CommandManagerTest, Bookmark_HandlesRemoveFlag) {
    EXPECT_CALL(mock_bookmark_manager, remove_bookmark("alias1"))
        .Times(2);

    std::string command1 = "bookmark --remove alias1";
    std::string command2 = "bookmark -r alias1";

    mock_command_manager.run_command(command1);
    mock_command_manager.run_command(command2);
}

// Test case: Detects and runs alias
TEST_F(CommandManagerTest, Bookmark_DetectAlias) {
    std::string alias = "alias1";

    mock_command_manager.run_command(alias);

    EXPECT_TRUE(output_stream.str().find("result1") != std::string::npos);
}

// Test case: Displays error for invalid bookmark command format.
TEST_F(CommandManagerTest, Bookmark_DisplaysErrorForInvalidCommand) {
    std::string command = "bookmark";
    mock_command_manager.run_command(command);
    EXPECT_TRUE(error_stream.str().find("Error") != std::string::npos);
}

// Test case: Correctly parses clear command
TEST_F(CommandManagerTest, Clear) {
    mock_agency_manager.session_history.emplace_back("query", "result");
    std::string command = "clear";

    mock_command_manager.run_command(command);
    EXPECT_TRUE(agency_manager.session_history.empty());
}

// Test case: Successfully reads from file.
TEST_F(CommandManagerTest, ReadFromFile) {
    // arrange a temp file
    std::string temp_filepath = "temp_file.txt";
    std::ofstream temp_file(temp_filepath);

    std::string expected_output =
        "This is a help documentation line.\n"
        "Another line of help documentation.\n";

    temp_file << expected_output;
    temp_file.close();

    std::string output;
    int rc = command_manager.read_from_file(temp_filepath, output);

    EXPECT_EQ(rc, 0);
    EXPECT_EQ(output, expected_output);
    // cleanup
    std::remove(temp_filepath.c_str());
}

// Test case: Correctly prints help data.
TEST_F(CommandManagerTest, Help) {
    EXPECT_CALL(mock_command_manager, read_from_file(_, _))
        .WillOnce(DoAll(
            SetArgReferee<1>(std::string("Help test.")),
            Return(0)));

    std::string command = "bookmark --help";
    mock_command_manager.run_command(command);

    EXPECT_TRUE(output_stream.str().find("Help test.") != std::string::npos);
}

// Test case: Displays error if help file not found.
TEST_F(CommandManagerTest, HelpError) {
    EXPECT_CALL(mock_command_manager, read_from_file(_, _))
        .Times(2).WillRepeatedly(Return(-1));

    std::string command = "bookmark --help";
    mock_command_manager.run_command(command);

    EXPECT_TRUE(error_stream.str().find("Error") != std::string::npos);
}

// Test case: Agent switching works correctly.
TEST_F(CommandManagerTest, Switch) {
    setenv("ISHELL_AGENCY_URL", "localhost", 1);

    std::string command = "switch inspector";
    mock_command_manager.run_command(command);

    unsetenv("ISHELL_AGENCY_ENV");

    EXPECT_EQ(mock_agency_manager.agent_name, "inspector");
}
