#include <gtest/gtest.h>
#include "../include/bookmarks.hpp"
#include "../include/assistant_query.hpp"

std::string mock_execute_query(const std::string& query, std::vector<std::pair<std::string, std::string>>& session_history) {
    for (const auto& session : session_history) {
        if (session.first == query) {
            return session.second;
        }
    }
    return "mocked_result";
}

class BookmarkTest : public ::testing::Test {
protected:
    void SetUp() override {
        bookmarks.clear();
    }

    void TearDown() override {
        bookmarks.clear();
    }
};

TEST(BookmarksTest, SuccessfullyBookmarksCommandUsingValidAliasAndIndex) {
    // arrange
    std::vector<std::pair<std::string, std::string>> session_history = {{"test_query", "test_result"}};
    bookmarks.clear(); // ensure bookmarks is empty
    // act
    bookmark(1, "test_alias", session_history);
    // assert
    ASSERT_TRUE(is_bookmark("test_alias"));
    EXPECT_EQ(get_bookmark("test_alias").first, "test_query");
    EXPECT_EQ(get_bookmark("test_alias").second, "test_result");
}

TEST(BookmarksTest, DetectsIfAliasAlreadyExists) {
    // arrange
    std::vector<std::pair<std::string, std::string>> session_history = {{"test_query", "test_result"}};
    bookmarks["existing_alias"] = {"existing_query", "existing_result"};
    // act
    testing::internal::CaptureStdout();
    bookmark(1, "existing_alias", session_history);
    std::string output = testing::internal::GetCapturedStdout();
    // assert
    EXPECT_NE(output.find("Error: Bookmark 'existing_alias' already exists."), std::string::npos);
}

TEST(BookmarksTest, HandlesInvalidHistoryIndex) {
    // arrange
    std::vector<std::pair<std::string, std::string>> session_history;
    // act
    testing::internal::CaptureStdout();
    bookmark(99, "test_alias", session_history);
    std::string output = testing::internal::GetCapturedStdout();
    // assert
    EXPECT_NE(output.find("Error: Invalid history index."), std::string::npos);
}

TEST(BookmarksTest, ExecutesQueryIfResultNotFoundInSessionHistory) {
    // arrange
    std::vector<std::pair<std::string, std::string>> session_history;
    bookmarks.clear(); // ensure bookmarks is empty
    std::string query_result = mock_execute_query("test_query", session_history);
    // act
    bookmark(1, "test_alias", session_history);
    // assert
    ASSERT_TRUE(is_bookmark("test_alias"));
    EXPECT_EQ(get_bookmark("test_alias").first, "test_query");
    EXPECT_EQ(get_bookmark("test_alias").second, query_result);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}