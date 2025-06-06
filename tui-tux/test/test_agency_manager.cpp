#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstdlib>
#include <vector>
#include <string>
#include <agency_manager.hpp>
#include <agency_request_wrapper.hpp>

#include <utils.hpp>

class MockAgencyRequestWrapper final : public AgencyRequestWrapper {
public:
    MOCK_METHOD(std::string, ask_agent, (const std::string& url, const std::string& query, (std::vector<std::pair<std::string, std::string>> &session_history)), (override));
};

class AgencyManagerTest : public ::testing::Test {
protected:
    MockAgencyRequestWrapper mock_request_wrapper;
    AgencyManager agency_manager;
    std::stringstream error_stream;
    std::streambuf *original_cerr;

    AgencyManagerTest() : agency_manager(&mock_request_wrapper), original_cerr(nullptr) {
    }

    void SetUp() override {
        original_cerr = std::cout.rdbuf();
        std::cerr.rdbuf(error_stream.rdbuf());

        // clear session history before each test
        agency_manager.session_history.clear();
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
    const std::string url = agency_manager.get_agency_url();
    // assert
    EXPECT_EQ(url, "http://localhost:5000");
}

// Test case: Correctly sets the agency URL when ISHELL_AGENCY_URL environment variable is not present.
TEST_F(AgencyManagerTest, GetAgencyUrl_NoUrlSet) {
    // act
    const std::string url = agency_manager.get_agency_url();
    // assert

    const std::string expected_agency(DEFAULT_AGENCY_URL);
    EXPECT_EQ(url, expected_agency);
}

// Test case: Successfully calls ask_agent and returns the agent's result when the agency URL is set.
TEST_F(AgencyManagerTest, ExecuteQuery_SuccessfullyCallsAgent) {
    // arrange
    setenv("ISHELL_AGENCY_URL", "http://localhost:5000", 1);
    const std::string url = agency_manager.get_agency_url();
    EXPECT_CALL(mock_request_wrapper, ask_agent("http://localhost:5000/assistant", "test query", agency_manager.session_history))
        .WillOnce(::testing::Return("agent response"));
    // act
    const std::string result = agency_manager.execute_query(url + "/assistant", "test query");
    // assert
    EXPECT_EQ(result, "agent response");
}

// Test case: Correctly adds the query and result to session_history after execution.
TEST_F(AgencyManagerTest, ExecuteQuery_AddsToSessionHistory) {
    // arrange
    setenv("ISHELL_AGENCY_URL", "http://localhost:5000", 1);
    const std::string url = agency_manager.get_agency_url();
    EXPECT_CALL(mock_request_wrapper, ask_agent("http://localhost:5000/assistant", "test query", agency_manager.session_history))
        .WillOnce(::testing::Return("agent response"));
    // act
    std::string result = agency_manager.execute_query(url + "/assistant", "test query");
    // assert
    ASSERT_EQ(agency_manager.session_history.size(), 1);
    EXPECT_EQ(agency_manager.session_history[0].first, "test query");
    EXPECT_EQ(agency_manager.session_history[0].second, "agent response");
}

// Test case: Returns an empty result if ask_agent fails.
TEST_F(AgencyManagerTest, ExecuteQuery_AskAgentFails) {
    // arrange
    setenv("ISHELL_AGENCY_URL", "http://localhost:5000", 1);
    const std::string url = agency_manager.get_agency_url();
    EXPECT_CALL(mock_request_wrapper, ask_agent("http://localhost:5000/assistant", "test query", agency_manager.session_history))
        .WillOnce(::testing::Return(""));
    // act
    const std::string result = agency_manager.execute_query(url + "/assistant", "test query");
    // assert
    EXPECT_EQ(result, "");
    EXPECT_EQ(agency_manager.session_history.size(), 1);  // history is still recorded
    EXPECT_EQ(agency_manager.session_history[0].first, "test query");
    EXPECT_EQ(agency_manager.session_history[0].second, "");
}
