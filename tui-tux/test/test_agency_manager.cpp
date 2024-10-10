#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstdlib>
#include <vector>
#include <string>
#include "../include/agency_manager.hpp"
#include "../include/agency_request_wrapper.hpp"

class MockAgencyRequestWrapper : public AgencyRequestWrapper {
public:
    MOCK_METHOD(std::string, ask_agent, (const std::string& url, const std::string& query), (override));
};

class AgencyManagerTest : public ::testing::Test {
protected:
    MockAgencyRequestWrapper mock_request_wrapper;
    AgencyManager agency_manager;
    std::vector<std::pair<std::string, std::string>> session_history;
    std::stringstream error_stream;
    std::streambuf *original_cerr;

    AgencyManagerTest() : agency_manager(&mock_request_wrapper) {}

    void SetUp() override {
        original_cerr = std::cout.rdbuf();
        std::cerr.rdbuf(error_stream.rdbuf());

        // clear session history before each test
        session_history.clear();
        unsetenv("ISHELL_AGENCY_URL");  // ensure environment variable is not set
    }

    void TearDown() override {
        std::cerr.rdbuf(original_cerr);
        // reset environment variable after each test
        unsetenv("ISHELL_AGENCY_URL");


    }
};

// Test case: Successfully sets the agency URL when ISHELL_AGENCY_URL environment variable is present.
TEST_F(AgencyManagerTest, GetAgencyUrl_SuccessfullySetsUrl) {
    // arrange
    setenv("ISHELL_AGENCY_URL", "http://localhost:5000", 1);
    // act
    agency_manager.get_agency_url();
    // assert
    EXPECT_TRUE(agency_manager.agency_url_set);
    EXPECT_EQ(agency_manager.agency_url, "http://localhost:5000");
    EXPECT_EQ(agency_manager.assistant_url, "http://localhost:5000/assistant");
}

// Test case: Does not set the agency URL when ISHELL_AGENCY_URL environment variable is not present.
TEST_F(AgencyManagerTest, GetAgencyUrl_NoUrlSet) {
    // act
    agency_manager.get_agency_url();
    // assert
    EXPECT_FALSE(agency_manager.agency_url_set);
    EXPECT_EQ(agency_manager.agency_url, "");
    EXPECT_EQ(agency_manager.assistant_url, "");
}

// Test case: Returns error message when ISHELL_AGENCY_URL is not set.
TEST_F(AgencyManagerTest, ExecuteQuery_AgencyUrlNotSet) {
    // act
    std::string result = agency_manager.execute_query("test query", session_history);
    // assert
    EXPECT_EQ(result, "");
    EXPECT_EQ(session_history.size(), 0);  // No history added
}

// Test case: Successfully calls ask_agent and returns the agent's result when the agency URL is set.
TEST_F(AgencyManagerTest, ExecuteQuery_SuccessfullyCallsAgent) {
    // arrange
    setenv("ISHELL_AGENCY_URL", "http://localhost:5000", 1);
    agency_manager.get_agency_url();
    EXPECT_CALL(mock_request_wrapper, ask_agent("http://localhost:5000/assistant", "test query"))
        .WillOnce(::testing::Return("agent response"));
    // act
    std::string result = agency_manager.execute_query("test query", session_history);
    // assert
    EXPECT_EQ(result, "agent response");
}

// Test case: Correctly adds the query and result to session_history after execution.
TEST_F(AgencyManagerTest, ExecuteQuery_AddsToSessionHistory) {
    // arrange
    setenv("ISHELL_AGENCY_URL", "http://localhost:5000", 1);
    agency_manager.get_agency_url();
    EXPECT_CALL(mock_request_wrapper, ask_agent("http://localhost:5000/assistant", "test query"))
        .WillOnce(::testing::Return("agent response"));
    // act
    std::string result = agency_manager.execute_query("test query", session_history);
    // assert
    ASSERT_EQ(session_history.size(), 1);
    EXPECT_EQ(session_history[0].first, "test query");
    EXPECT_EQ(session_history[0].second, "agent response");
}

// Test case: Returns an empty result if ask_agent fails.
TEST_F(AgencyManagerTest, ExecuteQuery_AskAgentFails) {
    // arrange
    setenv("ISHELL_AGENCY_URL", "http://localhost:5000", 1);
    agency_manager.get_agency_url();
    EXPECT_CALL(mock_request_wrapper, ask_agent("http://localhost:5000/assistant", "test query"))
        .WillOnce(::testing::Return(""));
    // act
    std::string result = agency_manager.execute_query("test query", session_history);
    // assert
    EXPECT_EQ(result, "");
    EXPECT_EQ(session_history.size(), 1);  // history is still recorded
    EXPECT_EQ(session_history[0].first, "test query");
    EXPECT_EQ(session_history[0].second, "");
}
