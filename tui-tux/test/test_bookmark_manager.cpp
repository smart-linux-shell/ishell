#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include <string>
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

    BookmarkTest() : mock_bookmark_manager(&mock_agency_manager) {}

    void SetUp() override {
        // redirect std::cout to output_stream
        std::cout.rdbuf(output_stream.rdbuf());

        mock_bookmark_manager.bookmarks.clear();
        mock_bookmark_manager.bookmarks["alias1"] = {"query1", "result1"};
        session_history.clear();
        session_history.push_back({"query2", "result2"});
        session_history.push_back({"query3", "result3"});
    }

    void TearDown() override {
        // reset the std::cout buffer back to default
        std::cout.rdbuf(nullptr);
    }
};

// Test case: Successfully bookmarks command using valid alias and index.
TEST_F(BookmarkTest, Bookmark_SuccessfullyAddsBookmark) {
    // setup mock expectations
    EXPECT_CALL(mock_bookmark_manager, get_query_from_history(1))
        .WillOnce(::testing::Return("query2"));
    EXPECT_CALL(mock_bookmark_manager, find_result_in_session_history("query2", ::testing::_))
        .WillOnce(::testing::Return("result2"));
    // act
    mock_bookmark_manager.bookmark(1, "alias2", session_history);
    // assert
    EXPECT_EQ(mock_bookmark_manager.bookmarks["alias2"].first, "query2");
    EXPECT_EQ(mock_bookmark_manager.bookmarks["alias2"].second, "result2");
    ASSERT_EQ(mock_bookmark_manager.bookmarks.size(), MOCK_BOOKMARS_SIZE + 1);
}

// Test case: Detects if the alias already exists and displays error.
TEST_F(BookmarkTest, Bookmark_AliasAlreadyExists) {
    // act
    mock_bookmark_manager.bookmark(1, "alias1", session_history);
    // assert
    EXPECT_EQ(output_stream.str(), "Error: Bookmark 'alias1' already exists.\n");
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
    EXPECT_EQ(output_stream.str(), "Error: Invalid history index.\n");
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
    EXPECT_EQ(output_stream.str(), "Error: Bookmark 'alias3' not found.\n");
    ASSERT_EQ(mock_bookmark_manager.bookmarks.size(), MOCK_BOOKMARS_SIZE - 0);
}
