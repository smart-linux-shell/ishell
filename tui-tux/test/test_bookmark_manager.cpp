#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <unordered_map>
#include <bookmark_manager.hpp>
#include <readline/history.h>

#define MOCK_BOOKMARS_SIZE 1

class MockAgencyManager : public AgencyManager {
public:
    MockAgencyManager(AgencyRequestWrapper* request_wrapper) : AgencyManager(request_wrapper) {}

    MOCK_METHOD(std::string, execute_query, (const std::string &query, (std::vector<std::pair<std::string, std::string>> &session_history)), (override));
};

class MockBaseBookmarkManager : public BookmarkManager {
public:
    MockBaseBookmarkManager(AgencyManager* agency_mgr) : BookmarkManager(agency_mgr) {}
};

class MockBookmarkManager : public BookmarkManager {
public:
    MockBookmarkManager(AgencyManager* agency_mgr) : BookmarkManager(agency_mgr) {}

    MOCK_METHOD(std::string, get_query_from_history, (int index), (override));
    MOCK_METHOD(std::string, find_result_in_session_history, (const std::string &query, (std::vector<std::pair<std::string, std::string>> &)), (override));
};

class BookmarkTest : public ::testing::Test {
protected:
    AgencyRequestWrapper request_wrapper;
    MockAgencyManager mock_agency_manager;
    MockBaseBookmarkManager mock_base_bookmark_manager;
    MockBookmarkManager mock_bookmark_manager;
    std::vector<std::pair<std::string, std::string>> session_history;

    std::stringstream output_stream;
    std::stringstream error_stream;

    std::streambuf *original_cout;
    std::streambuf *original_cerr;

    BookmarkTest()
        : mock_agency_manager(&request_wrapper),
          mock_base_bookmark_manager(&mock_agency_manager),
          mock_bookmark_manager(&mock_agency_manager) {}

    void SetUp() override {
        // redirect std::cout to output_stream
        original_cout = std::cout.rdbuf();
        original_cerr = std::cerr.rdbuf();

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
        std::cout.rdbuf(original_cout);
        std::cerr.rdbuf(original_cerr);
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


// Test case: Successfully creates a new bookmark file.
TEST_F(BookmarkTest, CreateBookmarksFile_SuccessfullyCreatesNewFile) {
    // arrange
    const std::string filename = "bookmarks_test.json";
    // act
    bool result = mock_bookmark_manager.create_bookmarks_file(filename);
    // assert
    EXPECT_TRUE(result);
    std::ifstream created_file(filename);
    EXPECT_TRUE(created_file.good());
    std::string file_content((std::istreambuf_iterator<char>(created_file)),
                              std::istreambuf_iterator<char>()); // empty json file
    EXPECT_EQ(file_content, "[]");
    // cleanup
    std::remove(filename.c_str());
}

// Test case: Fails to create a new bookmark file (invalid file path).
TEST_F(BookmarkTest, CreateBookmarksFile_FailsToCreateFileWithInvalidPath) {
    // act
    bool result = mock_bookmark_manager.create_bookmarks_file("/invalid/path/bookmarks_test.json");
    // assert
    EXPECT_FALSE(result);
    EXPECT_EQ(error_stream.str(), "Error creating file: /invalid/path/bookmarks_test.json\n");
}

// Test case: Successfully parses valid JSON object into bookmarks.
TEST_F(BookmarkTest, ParseBookmarkJson_SuccessfullyParsesValidJson) {
    // arrange
    json valid_json = {
        {"alias", "alias1"},
        {"query", "query1"},
        {"result", "result1"}
    };
    // act
    mock_bookmark_manager.parse_bookmark_json(valid_json);
    // assert
    EXPECT_EQ(mock_bookmark_manager.bookmarks["alias1"].first, "query1");
    EXPECT_EQ(mock_bookmark_manager.bookmarks["alias1"].second, "result1");
}

// Test case: Fails to parse invalid JSON object (missing fields).
TEST_F(BookmarkTest, ParseBookmarkJson_FailsToParseInvalidJson) {
    // arrange (missing result field)
    json invalid_json = {
        {"alias", "alias1"},
        {"query", "query1"}

    };
    // act & assert
    try {
        mock_bookmark_manager.parse_bookmark_json(invalid_json);
        FAIL() << "Expected exception due to missing fields in JSON.";
    } catch (const nlohmann::json::out_of_range& e) {
        EXPECT_STREQ(e.what(), "[json.exception.out_of_range.403] key 'result' not found");
    } catch (...) {
        FAIL() << "Expected nlohmann::json::out_of_range exception, but got a different type.";
    }
    EXPECT_EQ(mock_bookmark_manager.bookmarks.size(), MOCK_BOOKMARS_SIZE); // ensure no new bookmarks were added
}

// Test case: Successfully loads bookmarks from existing file.
TEST_F(BookmarkTest, LoadBookmarks_SucessfullyLoadsBookmarks) {
    // arrange
    std::string filename = "bookmarks.json";
    std::ofstream empty_file(filename);
    empty_file << "[{\"alias\": \"alias\", \"query\": \"query\", \"result\": \"result\"}]";
    empty_file.close();
    // act
    mock_bookmark_manager.load_bookmarks(filename);
    // assert
    EXPECT_EQ(mock_bookmark_manager.bookmarks.size(), MOCK_BOOKMARS_SIZE + 1);
    // cleanup
    std::remove(filename.c_str());
}

// Test case: Fails to load bookmarks if the file does not exist, and creates a new file.
TEST_F(BookmarkTest, LoadBookmarks_CreatesNewFileIfNotExist) {
    // arrange
    std::string filename = "non_existing_bookmarks.json";
    // act
    mock_bookmark_manager.load_bookmarks(filename);
    // assert
    EXPECT_EQ(mock_bookmark_manager.bookmarks.size(), MOCK_BOOKMARS_SIZE);
    // cleanup
    std::remove(filename.c_str());
}

// Test case: Empty bookmarks file is handled correctly.
TEST_F(BookmarkTest, LoadBookmarks_HandlesEmptyFileCorrectly) {
    // arrange
    std::string filename = "bookmarks.json";
    std::ofstream empty_file(filename);
    empty_file << "[]";
    empty_file.close();
    // act
    mock_bookmark_manager.load_bookmarks(filename);
    // assert
    EXPECT_EQ(mock_bookmark_manager.bookmarks.size(), MOCK_BOOKMARS_SIZE);
    // cleanup
    std::remove(filename.c_str());
}

// Test case: Successfully saves bookmarks to file.
TEST_F(BookmarkTest, SaveBookmarks_SuccessfullySavesToFile) {
    // arrange
    std::string filename = "bookmarks.json";
    // act
    mock_bookmark_manager.save_bookmarks(filename);
    // assert
    std::ifstream file(filename);
    ASSERT_TRUE(file.is_open());
    json bookmark_json;
    file >> bookmark_json;
    file.close();
    ASSERT_EQ(bookmark_json.size(), MOCK_BOOKMARS_SIZE);
    EXPECT_EQ(bookmark_json[0]["alias"], "alias1");
    EXPECT_EQ(bookmark_json[0]["query"], "query1");
    EXPECT_EQ(bookmark_json[0]["result"], "result1");
    // cleanup
    std::remove(filename.c_str());
}

// Test case: Fails to save bookmarks if the file cannot be opened (read-only file).
TEST_F(BookmarkTest, SaveBookmarks_FailsToOpenReadOnlyFile) {
    // arrange
    std::string filename = "bookmarks.json";
    std::ofstream file(filename);
    file.close();
    chmod(filename.c_str(), 0444);  // make the file read-only
    // act
    mock_bookmark_manager.save_bookmarks(filename);
    // assert
    EXPECT_TRUE(error_stream.str().find("Error opening file") != std::string::npos);
    // cleanup
    std::remove(filename.c_str());
}

// Test case: Detects valid bookmark commands.
TEST_F(BookmarkTest, IsBookmarkCommand_ValidCommand) {
    // assert
    EXPECT_TRUE(mock_bookmark_manager.is_bookmark_command("bookmark add alias"));
}

// Test case: Returns false for invalid commands.
TEST_F(BookmarkTest, IsBookmarkCommand_InvalidCommand) {
    // assert
    EXPECT_FALSE(mock_bookmark_manager.is_bookmark_command("invalid command"));
}

// Test case: Recognizes -b and --bookmark flags.
TEST_F(BookmarkTest, IsBookmarkFlag_ValidFlags) {
    // assert
    EXPECT_TRUE(mock_bookmark_manager.is_bookmark_flag("-b"));
    EXPECT_TRUE(mock_bookmark_manager.is_bookmark_flag("--bookmark"));
}

// Test case: Returns false for non-bookmark flags.
TEST_F(BookmarkTest, IsBookmarkFlag_InvalidFlags) {
    // assert
    EXPECT_FALSE(mock_bookmark_manager.is_bookmark_flag("-x"));
    EXPECT_FALSE(mock_bookmark_manager.is_bookmark_flag("--remove"));
}

// Test case: Recognizes -r and --remove flags.
TEST_F(BookmarkTest, IsRemoveFlag_ValidFlags) {
    // assert
    EXPECT_TRUE(mock_bookmark_manager.is_remove_flag("-r"));
    EXPECT_TRUE(mock_bookmark_manager.is_remove_flag("--remove"));
}

// Test case: Returns false for non-remove flags.
TEST_F(BookmarkTest, IsRemoveFlag_InvalidFlags) {
    // assert
    EXPECT_FALSE(mock_bookmark_manager.is_remove_flag("-b"));
    EXPECT_FALSE(mock_bookmark_manager.is_remove_flag("--bookmark"));
}

// Test case: Successfully detects existing bookmarks by alias.
TEST_F(BookmarkTest, IsBookmark_ExistingBookmark) {
    // assert
    EXPECT_TRUE(mock_bookmark_manager.is_bookmark("alias1"));
}

// Test case: Returns false for non-existent bookmarks.
TEST_F(BookmarkTest, IsBookmark_NonExistentBookmark) {
    // assert
    EXPECT_FALSE(mock_bookmark_manager.is_bookmark("non_existent_alias"));
}

// Test case: Retrieves bookmark by alias.
TEST_F(BookmarkTest, GetBookmark_ExistingAlias) {
    // act
    std::pair<std::string, std::string> bookmark = mock_bookmark_manager.get_bookmark("alias1");
    // assert
    EXPECT_EQ(bookmark.first, "query1");
    EXPECT_EQ(bookmark.second, "result1");
}

// Test case: Returns empty pair for non-existent alias.
TEST_F(BookmarkTest, GetBookmark_NonExistentAlias) {
    // act
    std::pair<std::string, std::string> bookmark = mock_bookmark_manager.get_bookmark("non_existent_alias");
    // assert
    EXPECT_EQ(bookmark.first, "");
    EXPECT_EQ(bookmark.second, "");
}

// Test case: Detects list flags.
TEST_F(BookmarkTest, IsListFlag_ValidFlags) {
    // assert
    EXPECT_TRUE(mock_bookmark_manager.is_list_flag("bookmark -l"));
    EXPECT_TRUE(mock_bookmark_manager.is_list_flag("bookmark --list"));
}

// Test case: Returns false for non-list flags.
TEST_F(BookmarkTest, IsListFlag_InvalidFlags) {
    // assert
    EXPECT_FALSE(mock_bookmark_manager.is_list_flag("bookmark -h"));
    EXPECT_FALSE(mock_bookmark_manager.is_list_flag("bookmark --help"));
}

// Test case: Detects help flags.
TEST_F(BookmarkTest, IsHelpFlag_ValidFlags) {
    // assert
    EXPECT_TRUE(mock_bookmark_manager.is_help_flag("bookmark -h"));
    EXPECT_TRUE(mock_bookmark_manager.is_help_flag("bookmark --help"));
}

// Test case: Returns false for non-help flags.
TEST_F(BookmarkTest, IsHelpFlag_InvalidFlags) {
    // assert
    EXPECT_FALSE(mock_bookmark_manager.is_help_flag("bookmark -l"));
    EXPECT_FALSE(mock_bookmark_manager.is_help_flag("bookmark --list"));
}

// Test case: Retrieves the correct query by index.
TEST_F(BookmarkTest, GetQueryFromHistory_ValidIndex) {
    // arrange
    history_length = 2;
    add_history("command2");
    // assert
    EXPECT_EQ(mock_base_bookmark_manager.get_query_from_history(1), "command2");
}

// Test case: Returns empty string for invalid index.
TEST_F(BookmarkTest, GetQueryFromHistory_InvalidIndex) {
    // assert
    EXPECT_EQ(mock_base_bookmark_manager.get_query_from_history(3), "");
}

// Test case: Finds correct result based on query.
TEST_F(BookmarkTest, FindResultInSessionHistory_ValidQuery) {
    // assert
    EXPECT_EQ(mock_base_bookmark_manager.find_result_in_session_history("query1", session_history), "result1");
    EXPECT_EQ(mock_base_bookmark_manager.find_result_in_session_history("query2", session_history), "result2");
}

// Test case: Returns empty string if query not found.
TEST_F(BookmarkTest, FindResultInSessionHistory_InvalidQuery) {
    // assert
    EXPECT_EQ(mock_base_bookmark_manager.find_result_in_session_history("non_existent_query", session_history), "");
}
