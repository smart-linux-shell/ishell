#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include <string>
#include <unordered_map>
#include "../include/bookmark_manager.hpp"

class MockBookmarkManager : public BookmarkManager {
public:
    MOCK_METHOD(std::string, get_query_from_history, (int index), ());
    MOCK_METHOD(std::string, find_result_in_session_history, (const std::string &query, (std::vector<std::pair<std::string, std::string>> &) session_history), ());
    MOCK_METHOD(std::string, execute_query, (const std::string &query, (std::vector<std::pair<std::string, std::string>> &) session_history), ());
};

class BookmarkTest : public ::testing::Test {
protected:
    MockBookmarkManager mock_bookmark_manager;
    std::vector<std::pair<std::string, std::string>> session_history;

    void SetUp() override {
        mock_bookmark_manager.bookmarks.clear();
    }
};

// Test case: Successfully bookmarks command using valid alias and index.
TEST_F(BookmarkTest, BookmarkCommand_SuccessfullyAddsBookmark) {
    // setup mock expectations
    EXPECT_CALL(mock_bookmark_manager, get_query_from_history(1))
        .WillOnce(::testing::Return("query1"));

    EXPECT_CALL(mock_bookmark_manager, find_result_in_session_history("query1", ::testing::_))
        .WillOnce(::testing::Return("result1"));
    // act
    mock_bookmark_manager.bookmark(1, "alias1", session_history);
    // assert
    ASSERT_EQ(mock_bookmark_manager.bookmarks.size(), 1);
    EXPECT_EQ(mock_bookmark_manager.bookmarks["alias1"].first, "query1");
    EXPECT_EQ(mock_bookmark_manager.bookmarks["alias1"].second, "result1");
}

// Test case: Detects if the alias already exists and displays error.
TEST_F(BookmarkTest, BookmarkCommand_AliasAlreadyExists) {
    // setup existing bookmark
    mock_bookmark_manager.bookmarks["alias1"] = {"query1", "result1"};
    // redirect output
    testing::internal::CaptureStdout();
    // act
    mock_bookmark_manager.bookmark(1, "alias1", session_history);
    // assert
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "Error: Bookmark 'alias1' already exists.\n");
    EXPECT_EQ(mock_bookmark_manager.bookmarks.size(), 1);
}

// Test case: Handles invalid history index.
//TEST_F(BookmarkTest, BookmarkCommand_InvalidHistoryIndex) {
//    // setup mock to return empty query for invalid index
//    EXPECT_CALL(mock_bookmark_manager, get_query_from_history(99))
//        .WillOnce(::testing::Return(""));
//    // redirect output
//    testing::internal::CaptureStdout();
//    // act
//    bookmark(99, "alias1", session_history);
//    // assert
//    std::string output = testing::internal::GetCapturedStdout();
//    EXPECT_EQ(output, "Error: Invalid history index.\n");
//    EXPECT_TRUE(bookmarks.empty());
//}
//
//// Test case: Executes the query if the result is not found in session history.
//TEST_F(BookmarkTest, BookmarkCommand_ExecutesQueryIfResultNotFound) {
//    // setup mock to return a query and simulate result not found in session history
//    EXPECT_CALL(mock_bookmark_manager, get_query_from_history(1))
//        .WillOnce(::testing::Return("query1"));
//    EXPECT_CALL(mock_bookmark_manager, find_result_in_session_history("query1", ::testing::_))
//        .WillOnce(::testing::Return(""));  // Simulate result not found
//    EXPECT_CALL(mock_bookmark_manager, execute_query("query1", ::testing::_))
//        .WillOnce(::testing::Return("executed_result"));
//    // act
//    bookmark(1, "alias2", session_history);
//    // assert
//    ASSERT_EQ(bookmarks.size(), 1);
//    EXPECT_EQ(bookmarks["alias2"].first, "query1");
//    EXPECT_EQ(bookmarks["alias2"].second, "executed_result");
//}
