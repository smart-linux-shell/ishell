#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <unordered_map>
#include "../include/bookmark_manager.hpp"

#define MOCK_BOOKMARS_SIZE 1

class MockAgencyManager : public AgencyManager {
public:
    MOCK_METHOD(std::string, execute_query, (const std::string &query, (std::vector<std::pair<std::string, std::string>> &session_history)), (override));
};

class MockBookmarkManager : public BookmarkManager {
public:
    MockBookmarkManager(AgencyManager* agency_mgr) : BookmarkManager(agency_mgr) {}

    MOCK_METHOD(std::string, get_query_from_history, (int index), (override));
    MOCK_METHOD(std::string, find_result_in_session_history, (const std::string &query, (std::vector<std::pair<std::string, std::string>> &)), (override));
};

class BookmarkTest : public ::testing::Test {
protected:
    MockAgencyManager mock_agency_manager;
    MockBookmarkManager mock_bookmark_manager;
    std::vector<std::pair<std::string, std::string>> session_history;

    std::stringstream output_stream;
    std::stringstream error_stream;

    BookmarkTest() : mock_bookmark_manager(&mock_agency_manager) {}

    void SetUp() override {
        // redirect std::cout to output_stream
        std::cout.rdbuf(output_stream.rdbuf());
        std::cerr.rdbuf(error_stream.rdbuf());

        mock_bookmark_manager.bookmarks.clear();
        mock_bookmark_manager.bookmarks["alias1"] = {"query1", "result1"};
        session_history.clear();
        session_history.push_back({"query1", "result1"});
        session_history.push_back({"query2", "result2"});
    }

    void TearDown() override {
        // reset the std::cout buffer back to default
        std::cout.rdbuf(nullptr);
        std::cerr.rdbuf(nullptr);
    }
};

// Test case: Successfully bookmarks command using valid alias and index.
TEST_F(BookmarkTest, Bookmark_SuccessfullyAddsBookmark) {
    // setup mock expectations
    EXPECT_CALL(mock_bookmark_manager, get_query_from_history(1))
        .WillOnce(::testing::Return("query1"));
    EXPECT_CALL(mock_bookmark_manager, find_result_in_session_history("query1", ::testing::_))
        .WillOnce(::testing::Return("result1"));
    // act
    mock_bookmark_manager.bookmark(1, "alias", session_history);
    // assert
    EXPECT_EQ(mock_bookmark_manager.bookmarks["alias1"].first, "query1");
    EXPECT_EQ(mock_bookmark_manager.bookmarks["alias1"].second, "result1");
    ASSERT_EQ(mock_bookmark_manager.bookmarks.size(), MOCK_BOOKMARS_SIZE + 1);
}

// Test case: Detects if the alias already exists and displays error.
TEST_F(BookmarkTest, Bookmark_AliasAlreadyExists) {
    // act
    mock_bookmark_manager.bookmark(1, "alias1", session_history);
    // assert
    EXPECT_EQ(error_stream.str(), "Error: Bookmark 'alias1' already exists.\n");
    EXPECT_EQ(mock_bookmark_manager.bookmarks.size(), MOCK_BOOKMARS_SIZE + 0);
}

// Test case: Handles invalid history index.
TEST_F(BookmarkTest, Bookmark_InvalidHistoryIndex) {
    // setup mock to return empty query for invalid index
    EXPECT_CALL(mock_bookmark_manager, get_query_from_history(99))
        .WillOnce(::testing::Return(""));
    // act
    mock_bookmark_manager.bookmark(99, "alias2", session_history);
    // assert
    EXPECT_EQ(error_stream.str(), "Error: Invalid history index.\n");
    EXPECT_EQ(mock_bookmark_manager.bookmarks.size(), MOCK_BOOKMARS_SIZE + 0);
}

// Test case: Executes the query if the result is not found in session history.
TEST_F(BookmarkTest, Bookmark_ExecutesQueryIfResultNotFound) {
    // setup mock to return a query and simulate result not found in session history
    EXPECT_CALL(mock_bookmark_manager, get_query_from_history(1))
        .WillOnce(::testing::Return("query2"));
    EXPECT_CALL(mock_bookmark_manager, find_result_in_session_history("query2", ::testing::_))
        .WillOnce(::testing::Return(""));  // Simulate result not found
    EXPECT_CALL(mock_agency_manager, execute_query("query2", ::testing::_))
        .WillOnce(::testing::Return("executed_result"));
    // act
    mock_bookmark_manager.bookmark(1, "alias2", session_history);
    // assert
    EXPECT_EQ(mock_bookmark_manager.bookmarks["alias2"].first, "query2");
    EXPECT_EQ(mock_bookmark_manager.bookmarks["alias2"].second, "executed_result");
    ASSERT_EQ(mock_bookmark_manager.bookmarks.size(), MOCK_BOOKMARS_SIZE + 1);
}

// Test case: Successfully removes an existing bookmark.
TEST_F(BookmarkTest, RemoveBookmark_SuccessfullyRemovesExistingBookmark) {
    // act
    mock_bookmark_manager.remove_bookmark("alias1");
    // assert
    EXPECT_EQ(mock_bookmark_manager.bookmarks.find("alias1"), mock_bookmark_manager.bookmarks.end());
    EXPECT_EQ(output_stream.str(), "Removed bookmark 'alias1'.\n");
    ASSERT_EQ(mock_bookmark_manager.bookmarks.size(), MOCK_BOOKMARS_SIZE - 1);
}

// Test case: Displays error if the alias does not exist.
TEST_F(BookmarkTest, RemoveBookmark_ErrorWhenAliasNotFound) {
    // act
    mock_bookmark_manager.remove_bookmark("alias3");
    // assert
    EXPECT_EQ(error_stream.str(), "Error: Bookmark 'alias3' not found.\n");
    ASSERT_EQ(mock_bookmark_manager.bookmarks.size(), MOCK_BOOKMARS_SIZE - 0);
}

// Test case: Lists all existing bookmarks in the correct format.
TEST_F(BookmarkTest, ListBookmarks_ListsAllExistingBookmarksCorrectly) {
    // act
    mock_bookmark_manager.list_bookmarks();
    // assert
    std::string expected_output =
        "BOOKMARK            QUERY                                             \n"
        "alias1              query1                                            \n";
    EXPECT_EQ(output_stream.str(), expected_output);
}

// Test case: Successfully loads and displays the help documentation.
TEST_F(BookmarkTest, Help_DisplaysHelpDocumentationSuccessfully) {
    // arrange a temp help file
    const std::string temp_help_filepath = "manuals/temp_bookmark.txt";
    std::ofstream temp_help_file(temp_help_filepath);
    temp_help_file << "This is a help documentation line.\n"
                      "Another line of help documentation.\n";
    temp_help_file.close();
    // act
    mock_bookmark_manager.help(temp_help_filepath);
    // assert
    std::string expected_output =
        "This is a help documentation line.\n"
        "Another line of help documentation.\n";
    EXPECT_EQ(output_stream.str(), expected_output);
    // cleanup
    std::remove(temp_help_filepath.c_str());
}

// Test case: Displays error if the documentation file cannot be found.
TEST_F(BookmarkTest, Help_DisplaysErrorWhenFileNotFound) {
    // act
    mock_bookmark_manager.help("manuals/temp_bookmark.txt");
    // assert
    EXPECT_EQ(error_stream.str(), "Error: Could not find the documentation.\n");
}

// Test case: Correctly parses valid bookmark command with index and alias.
TEST_F(BookmarkTest, TryParseBookmarkCommand_ValidWithIndexAndAlias) {
    // act
    std::string cmd; int index; std::string alias;
    bool result = mock_bookmark_manager.try_parse_bookmark_command("bookmark 2 alias", cmd, index, alias);
    // assert
    EXPECT_TRUE(result);
    EXPECT_EQ(cmd, "bookmark");
    EXPECT_EQ(index, 2);
    EXPECT_EQ(alias, "alias");
}

// Test case: Correctly parses valid bookmark command with alias only.
TEST_F(BookmarkTest, TryParseBookmarkCommand_ValidWithAliasOnly) {
    // act
    std::string cmd; int index; std::string alias;
    bool result = mock_bookmark_manager.try_parse_bookmark_command("bookmark alias", cmd, index, alias);
    // assert
    EXPECT_TRUE(result);
    EXPECT_EQ(cmd, "bookmark");
    EXPECT_EQ(index, 1);  // default index
    EXPECT_EQ(alias, "alias");
}

// Test case: Fails to parse invalid bookmark command.
TEST_F(BookmarkTest, TryParseBookmarkCommand_InvalidCommand) {
    // act
    std::string cmd; int index; std::string alias;
    bool result = mock_bookmark_manager.try_parse_bookmark_command("invalid_command alias", cmd, index, alias);
    // assert
    EXPECT_FALSE(result);
}

// Test case: Correctly handles list flag and lists bookmarks.
TEST_F(BookmarkTest, HandleBookmarkCommand_HandlesListFlag) {
    // act
    mock_bookmark_manager.handle_bookmark_command("bookmark --list", session_history);
    // assert
    std::string expected_output =
        "BOOKMARK            QUERY                                             \n"
        "alias1              query1                                            \n";
    EXPECT_EQ(output_stream.str(), expected_output);
}

// Test case: Correctly handles help flag and displays help.
TEST_F(BookmarkTest, HandleBookmarkCommand_DisplaysHelp) {
    // act
    mock_bookmark_manager.handle_bookmark_command("bookmark --help", session_history);
    // assert
    EXPECT_NE(error_stream.str(), "Error: Could not find the documentation.");
}

// Test case: Correctly handles valid bookmark command and bookmarks query.
TEST_F(BookmarkTest, HandleBookmarkCommand_HandlesValidBookmarkCommand) {
    // setup mock expectations
    EXPECT_CALL(mock_bookmark_manager, get_query_from_history(1))
        .WillOnce(::testing::Return("query1"));
    EXPECT_CALL(mock_bookmark_manager, find_result_in_session_history("query1", ::testing::_))
        .WillOnce(::testing::Return("result1"));
    // act
    mock_bookmark_manager.handle_bookmark_command("bookmark 1 alias", session_history);
    // assert
    EXPECT_EQ(mock_bookmark_manager.bookmarks["alias"].first, "query1");
    EXPECT_EQ(mock_bookmark_manager.bookmarks["alias"].second, "result1");
    ASSERT_EQ(mock_bookmark_manager.bookmarks.size(), MOCK_BOOKMARS_SIZE + 1);
}

// Test case: Displays error for invalid bookmark command format.
TEST_F(BookmarkTest, HandleBookmarkCommand_DisplaysErrorForInvalidCommand) {
    // act
    mock_bookmark_manager.handle_bookmark_command("invalid command", session_history);
    // assert
    EXPECT_EQ(error_stream.str(), "Error: Invalid bookmark command format.\n");
}
