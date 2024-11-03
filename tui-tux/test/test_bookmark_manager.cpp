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

class MockAgencyManager final : public AgencyManager {
public:
    explicit MockAgencyManager(AgencyRequestWrapper* request_wrapper) : AgencyManager(request_wrapper) {}

    MOCK_METHOD(std::string, execute_query, (const std::string &endpoint, const std::string &query), (override));
};

class MockBaseBookmarkManager final : public BookmarkManager {
public:
    explicit MockBaseBookmarkManager(AgencyManager* agency_mgr) : BookmarkManager(agency_mgr) {}
};

class BookmarkTest : public ::testing::Test {
protected:
    AgencyRequestWrapper request_wrapper;
    MockAgencyManager mock_agency_manager;
    MockBaseBookmarkManager mock_base_bookmark_manager;

    std::stringstream output_stream;
    std::stringstream error_stream;

    std::streambuf *original_cout{};
    std::streambuf *original_cerr{};

    BookmarkTest()
        : mock_agency_manager(&request_wrapper),
          mock_base_bookmark_manager(&mock_agency_manager) {}

    void SetUp() override {
        // redirect std::cout to output_stream
        original_cout = std::cout.rdbuf();
        original_cerr = std::cerr.rdbuf();

        std::cout.rdbuf(output_stream.rdbuf());
        std::cerr.rdbuf(error_stream.rdbuf());

        mock_base_bookmark_manager.bookmarks.clear();
        mock_base_bookmark_manager.bookmarks["alias1"] = {"query1", "result1"};
        mock_agency_manager.session_history.clear();
        mock_agency_manager.session_history.emplace_back("query1", "result1");
        mock_agency_manager.session_history.emplace_back("query2", "result2");
    }

    void TearDown() override {
        // reset the std::cout buffer back to default
        std::cout.rdbuf(original_cout);
        std::cerr.rdbuf(original_cerr);
    }
};


// Test case: Successfully bookmarks command using valid alias and index.
TEST_F(BookmarkTest, Bookmark_SuccessfullyAddsBookmark) {
    // act
    mock_base_bookmark_manager.bookmark(1, "alias");
    // assert
    EXPECT_EQ(mock_base_bookmark_manager.bookmarks["alias"].first, "query2");
    EXPECT_EQ(mock_base_bookmark_manager.bookmarks["alias"].second, "result2");
    ASSERT_EQ(mock_base_bookmark_manager.bookmarks.size(), MOCK_BOOKMARS_SIZE + 1);
}

// Test case: Detects if the alias already exists and displays error.
TEST_F(BookmarkTest, Bookmark_AliasAlreadyExists) {
    // act
    mock_base_bookmark_manager.bookmark(1, "alias1");
    // assert
    EXPECT_EQ(error_stream.str(), "Error: Bookmark 'alias1' already exists.\n");
    EXPECT_EQ(mock_base_bookmark_manager.bookmarks.size(), MOCK_BOOKMARS_SIZE + 0);
}

// Test case: Handles invalid history index.
TEST_F(BookmarkTest, Bookmark_InvalidHistoryIndex) {
    // act
    mock_base_bookmark_manager.bookmark(99, "alias2");
    // assert
    EXPECT_EQ(error_stream.str(), "Error: Invalid history index.\n");
    EXPECT_EQ(mock_base_bookmark_manager.bookmarks.size(), MOCK_BOOKMARS_SIZE + 0);
}

// Test case: Successfully removes an existing bookmark.
TEST_F(BookmarkTest, RemoveBookmark_SuccessfullyRemovesExistingBookmark) {
    // act
    mock_base_bookmark_manager.remove_bookmark("alias1");
    // assert
    EXPECT_EQ(mock_base_bookmark_manager.bookmarks.find("alias1"), mock_base_bookmark_manager.bookmarks.end());
    EXPECT_EQ(output_stream.str(), "Removed bookmark 'alias1'.\n");
    ASSERT_EQ(mock_base_bookmark_manager.bookmarks.size(), MOCK_BOOKMARS_SIZE - 1);
}

// Test case: Displays error if the alias does not exist.
TEST_F(BookmarkTest, RemoveBookmark_ErrorWhenAliasNotFound) {
    // act
    mock_base_bookmark_manager.remove_bookmark("alias3");
    // assert
    EXPECT_EQ(error_stream.str(), "Error: Bookmark 'alias3' not found.\n");
    ASSERT_EQ(mock_base_bookmark_manager.bookmarks.size(), MOCK_BOOKMARS_SIZE - 0);
}

// Test case: Lists all existing bookmarks in the correct format.
TEST_F(BookmarkTest, ListBookmarks_ListsAllExistingBookmarksCorrectly) {
    // act
    mock_base_bookmark_manager.list_bookmarks();
    // assert
    const std::string expected_output =
        "BOOKMARK            QUERY                                             \n"
        "alias1              query1                                            \n";
    EXPECT_EQ(output_stream.str(), expected_output);
}

// Test case: Successfully creates a new bookmark file.
TEST_F(BookmarkTest, CreateBookmarksFile_SuccessfullyCreatesNewFile) {
    // arrange
    const std::string filename = "bookmarks_test.json";
    // act
    bool result = mock_base_bookmark_manager.create_bookmarks_file(filename);
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
    const bool result = mock_base_bookmark_manager.create_bookmarks_file("/invalid/path/bookmarks_test.json");
    // assert
    EXPECT_FALSE(result);
    EXPECT_EQ(error_stream.str(), "Error creating file: /invalid/path/bookmarks_test.json\n");
}

// Test case: Successfully parses valid JSON object into bookmarks.
TEST_F(BookmarkTest, ParseBookmarkJson_SuccessfullyParsesValidJson) {
    // arrange
    const json valid_json = {
        {"alias", "alias1"},
        {"query", "query1"},
        {"result", "result1"}
    };
    // act
    mock_base_bookmark_manager.parse_bookmark_json(valid_json);
    // assert
    EXPECT_EQ(mock_base_bookmark_manager.bookmarks["alias1"].first, "query1");
    EXPECT_EQ(mock_base_bookmark_manager.bookmarks["alias1"].second, "result1");
}

// Test case: Fails to parse invalid JSON object (missing fields).
TEST_F(BookmarkTest, ParseBookmarkJson_FailsToParseInvalidJson) {
    // arrange (missing result field)
    const json invalid_json = {
        {"alias", "alias1"},
        {"query", "query1"}

    };
    // act & assert
    try {
        mock_base_bookmark_manager.parse_bookmark_json(invalid_json);
        FAIL() << "Expected exception due to missing fields in JSON.";
    } catch (const nlohmann::json::out_of_range& e) {
        EXPECT_STREQ(e.what(), "[json.exception.out_of_range.403] key 'result' not found");
    } catch (...) {
        FAIL() << "Expected nlohmann::json::out_of_range exception, but got a different type.";
    }
    EXPECT_EQ(mock_base_bookmark_manager.bookmarks.size(), MOCK_BOOKMARS_SIZE); // ensure no new bookmarks were added
}

// Test case: Successfully loads bookmarks from existing file.
TEST_F(BookmarkTest, LoadBookmarks_SucessfullyLoadsBookmarks) {
    // arrange
    const std::string filename = "bookmarks.json";
    std::ofstream empty_file(filename);
    empty_file << R"([{"alias": "alias", "query": "query", "result": "result"}])";
    empty_file.close();
    // act
    mock_base_bookmark_manager.load_bookmarks(filename);
    // assert
    EXPECT_EQ(mock_base_bookmark_manager.bookmarks.size(), MOCK_BOOKMARS_SIZE + 1);
    // cleanup
    std::remove(filename.c_str());
}

// Test case: Fails to load bookmarks if the file does not exist, and creates a new file.
TEST_F(BookmarkTest, LoadBookmarks_CreatesNewFileIfNotExist) {
    // arrange
    const std::string filename = "non_existing_bookmarks.json";
    // act
    mock_base_bookmark_manager.load_bookmarks(filename);
    // assert
    EXPECT_EQ(mock_base_bookmark_manager.bookmarks.size(), MOCK_BOOKMARS_SIZE);
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
    mock_base_bookmark_manager.load_bookmarks(filename);
    // assert
    EXPECT_EQ(mock_base_bookmark_manager.bookmarks.size(), MOCK_BOOKMARS_SIZE);
    // cleanup
    std::remove(filename.c_str());
}

// Test case: Successfully saves bookmarks to file.
TEST_F(BookmarkTest, SaveBookmarks_SuccessfullySavesToFile) {
    // arrange
    std::string filename = "bookmarks.json";
    // act
    mock_base_bookmark_manager.save_bookmarks(filename);
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
    const std::string filename = "bookmarks.json";
    std::ofstream file(filename);
    file.close();
    chmod(filename.c_str(), 0444);  // make the file read-only
    // act
    mock_base_bookmark_manager.save_bookmarks(filename);
    // assert
    EXPECT_TRUE(error_stream.str().find("Error opening file") != std::string::npos);
    // cleanup
    std::remove(filename.c_str());
}

// Test case: Successfully detects existing bookmarks by alias.
TEST_F(BookmarkTest, IsBookmark_ExistingBookmark) {
    // assert
    EXPECT_TRUE(mock_base_bookmark_manager.is_bookmark("alias1"));
}

// Test case: Returns false for non-existent bookmarks.
TEST_F(BookmarkTest, IsBookmark_NonExistentBookmark) {
    // assert
    EXPECT_FALSE(mock_base_bookmark_manager.is_bookmark("non_existent_alias"));
}

// Test case: Retrieves bookmark by alias.
TEST_F(BookmarkTest, GetBookmark_ExistingAlias) {
    // act
    auto [fst, snd] = mock_base_bookmark_manager.get_bookmark("alias1");
    // assert
    EXPECT_EQ(fst, "query1");
    EXPECT_EQ(snd, "result1");
}

// Test case: Returns empty pair for non-existent alias.
TEST_F(BookmarkTest, GetBookmark_NonExistentAlias) {
    // act
    auto [fst, snd] = mock_base_bookmark_manager.get_bookmark("non_existent_alias");
    // assert
    EXPECT_EQ(fst, "");
    EXPECT_EQ(snd, "");
}