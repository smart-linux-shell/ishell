#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <agency_request_wrapper.hpp>
#include <https_client.hpp>

using namespace testing;

class MockAgencyRequestWrapper1 : public AgencyRequestWrapper {
public:
    MOCK_METHOD(json, make_http_request, (HttpRequestType request_type, const std::string& url,
                       (const std::map<std::string, std::string>&) query_params,
                       const json& body,
                       (const std::map<std::string, std::string>&) headers), (override));
    MOCK_METHOD(std::string, get_linux_distro, (), (override));
    MOCK_METHOD(std::vector<std::string>, get_installed_packages, (), (override));
    MOCK_METHOD(std::string, get_ssh_ip, (), (override));
    MOCK_METHOD(int, get_ssh_port, (), (override));
    MOCK_METHOD(std::string, get_ssh_user, (), (override));
};

class MockAgencyRequestWrapper2 : public AgencyRequestWrapper {
public:
    MOCK_METHOD(json, make_http_request, (HttpRequestType request_type, const std::string& url,
                       (const std::map<std::string, std::string>&) query_params,
                       const json& body,
                       (const std::map<std::string, std::string>&) headers), (override));
    MOCK_METHOD(char *, getenv, (const char *), (override));
};

class AgencyRequestWrapperTest : public Test {
protected:
    MockAgencyRequestWrapper1 mock_agency_request_wrapper1;
    MockAgencyRequestWrapper2 mock_agency_request_wrapper2;

    std::string distro = "Test Distro";
    std::vector<std::string> packages = {"Test Package"};
    std::string ssh_ip = "0.0.0.0";
    int ssh_port = 0;
    std::string ssh_user = "test_user";

    std::string url = "0.0.0.1";
    std::string query = "Test Query";

    std::vector<std::pair<std::string, std::string>> session_history;
};

// Test case: Successfully creates a request with the correct data fields (distro, installed_packages, query, ssh_ip, ssh_port, ssh_user).
TEST_F(AgencyRequestWrapperTest, CorrectRequestData) {
    EXPECT_CALL(mock_agency_request_wrapper1, get_linux_distro())
        .WillOnce(Return(distro));
    EXPECT_CALL(mock_agency_request_wrapper1, get_installed_packages())
        .WillOnce(Return(packages));
    EXPECT_CALL(mock_agency_request_wrapper1, get_ssh_ip())
        .WillOnce(Return(ssh_ip));
    EXPECT_CALL(mock_agency_request_wrapper1, get_ssh_port())
        .WillOnce(Return(ssh_port));
    EXPECT_CALL(mock_agency_request_wrapper1, get_ssh_user())
        .WillOnce(Return(ssh_user));

    json request_body = {
        {"distro", distro},
        {"installed_packages", packages},
        {"query", query},
        {"ssh_ip", ssh_ip},
        {"ssh_port", ssh_port},
        {"ssh_user", ssh_user},
        {"session_history", ""}
    };

    json body = {
        {"content", "Test content"}
    };

    json response = {
        {"status_code", "200"},
        {"headers", {}},
        {"body", body}
    };
        
    EXPECT_CALL(mock_agency_request_wrapper1, make_http_request(HttpRequestType::POST, url, _, request_body, _))
        .WillOnce(Return(response));

    // Call
    mock_agency_request_wrapper1.send_request_to_agent_server(url, query, session_history);
};

// Test case: Handles the case where one or more fields are empty.
TEST_F(AgencyRequestWrapperTest, EmptyFields) {
    EXPECT_CALL(mock_agency_request_wrapper2, getenv(StrEq("SSH_IP")))
        .WillOnce(Return(nullptr));
    
    EXPECT_CALL(mock_agency_request_wrapper2, getenv(StrEq("SSH_PORT")))
        .WillOnce(Return(nullptr));

    EXPECT_CALL(mock_agency_request_wrapper2, getenv(StrEq("USER")))
        .WillOnce(Return(nullptr));

    EXPECT_CALL(mock_agency_request_wrapper2, getenv(StrEq("ISHELL_TOKEN")))
        .WillOnce(Return(nullptr));

    json request;

    json body = {
        {"content", "Test content"}
    };

    json response = {
        {"status_code", "200"},
        {"headers", {}},
        {"body", body}
    };

    EXPECT_CALL(mock_agency_request_wrapper2, make_http_request(HttpRequestType::POST, url, _, _, _))
        .WillOnce(DoAll(SaveArg<3>(&request), Return(response)));

    // Call
    mock_agency_request_wrapper2.send_request_to_agent_server(url, query, session_history);

    // Assert
    EXPECT_TRUE(request.contains("ssh_ip"));
    EXPECT_TRUE(request.contains("ssh_ip"));
    EXPECT_TRUE(request.contains("ssh_ip"));

    EXPECT_EQ(request["ssh_ip"], "Unknown");
    EXPECT_EQ(request["ssh_user"], "Unknown");
    EXPECT_EQ(request["ssh_port"], 22);
}

// Test case: Makes a POST request with the correct headers.
TEST_F(AgencyRequestWrapperTest, CorrectHeaders) {
    EXPECT_CALL(mock_agency_request_wrapper1, get_linux_distro())
        .WillOnce(Return(distro));
    EXPECT_CALL(mock_agency_request_wrapper1, get_installed_packages())
        .WillOnce(Return(packages));
    EXPECT_CALL(mock_agency_request_wrapper1, get_ssh_ip())
        .WillOnce(Return(ssh_ip));
    EXPECT_CALL(mock_agency_request_wrapper1, get_ssh_port())
        .WillOnce(Return(ssh_port));
    EXPECT_CALL(mock_agency_request_wrapper1, get_ssh_user())
        .WillOnce(Return(ssh_user));

    std::map<std::string, std::string> headers = {
        {"Content-Type", "application/json"}
    };

    json body = {
        {"content", "Test content"}
    };

    json response = {
        {"status_code", "200"},
        {"headers", {}},
        {"body", body}
    };
    
    EXPECT_CALL(mock_agency_request_wrapper1, make_http_request(HttpRequestType::POST, url, _, _, headers))
        .WillOnce(Return(response));

    // Call
    mock_agency_request_wrapper1.send_request_to_agent_server(url, query, session_history);
}

// Test case: Successfully retrieves content from the server response when the request is successful.
TEST_F(AgencyRequestWrapperTest, SuccessfulRequest) {
    EXPECT_CALL(mock_agency_request_wrapper1, get_linux_distro())
        .WillOnce(Return(distro));
    EXPECT_CALL(mock_agency_request_wrapper1, get_installed_packages())
        .WillOnce(Return(packages));
    EXPECT_CALL(mock_agency_request_wrapper1, get_ssh_ip())
        .WillOnce(Return(ssh_ip));
    EXPECT_CALL(mock_agency_request_wrapper1, get_ssh_port())
        .WillOnce(Return(ssh_port));
    EXPECT_CALL(mock_agency_request_wrapper1, get_ssh_user())
        .WillOnce(Return(ssh_user));

    std::string response_content = "Test content";

    json body = {
        {"content", response_content}
    };

    json response = {
        {"status_code", "200"},
        {"headers", {}},
        {"body", body}
    };

    EXPECT_CALL(mock_agency_request_wrapper1, make_http_request(_, _, _, _, _))
        .WillOnce(Return(response));

    // Call
    std::string agent_response = mock_agency_request_wrapper1.ask_agent(url, query, session_history);

    // Assert
    EXPECT_EQ(agent_response, response_content);
};

// Test case: Handles errors when the server returns an error field in the response.
TEST_F(AgencyRequestWrapperTest, ResponseError) {
    EXPECT_CALL(mock_agency_request_wrapper1, get_linux_distro())
        .WillOnce(Return(distro));
    EXPECT_CALL(mock_agency_request_wrapper1, get_installed_packages())
        .WillOnce(Return(packages));
    EXPECT_CALL(mock_agency_request_wrapper1, get_ssh_ip())
        .WillOnce(Return(ssh_ip));
    EXPECT_CALL(mock_agency_request_wrapper1, get_ssh_port())
        .WillOnce(Return(ssh_port));
    EXPECT_CALL(mock_agency_request_wrapper1, get_ssh_user())
        .WillOnce(Return(ssh_user));

    std::string response_error = "Test error";

    json body = {
        {"error", response_error}
    };

    json response = {
        {"status_code", "200"},
        {"headers", {}},
        {"body", body}
    };

    EXPECT_CALL(mock_agency_request_wrapper1, make_http_request(_, _, _, _, _))
        .WillOnce(Return(response));

    // Capture stderr
    internal::CaptureStderr();

    // Call
    mock_agency_request_wrapper1.ask_agent(url, query, session_history);

    std::string result_err = internal::GetCapturedStderr();

    // Assert
    EXPECT_TRUE(result_err.find(response_error) != std::string::npos);
};

// Test case: Returns an error message if the content field is not found in the server response.
TEST_F(AgencyRequestWrapperTest, ResponseNoContent) {
    EXPECT_CALL(mock_agency_request_wrapper1, get_linux_distro())
        .WillOnce(Return(distro));
    EXPECT_CALL(mock_agency_request_wrapper1, get_installed_packages())
        .WillOnce(Return(packages));
    EXPECT_CALL(mock_agency_request_wrapper1, get_ssh_ip())
        .WillOnce(Return(ssh_ip));
    EXPECT_CALL(mock_agency_request_wrapper1, get_ssh_port())
        .WillOnce(Return(ssh_port));
    EXPECT_CALL(mock_agency_request_wrapper1, get_ssh_user())
        .WillOnce(Return(ssh_user));

    json response_body = {};            // Empty (no content)

    json response = {
        {"status_code", "200"},
        {"headers", {}},
        {"body", response_body}
    };

    EXPECT_CALL(mock_agency_request_wrapper1, make_http_request(_, _, _, _, _))
        .WillOnce(Return(response));

    // Capture stderr
    internal::CaptureStderr();

    // Call
    mock_agency_request_wrapper1.ask_agent(url, query, session_history);

    std::string result_err = internal::GetCapturedStderr();

    // Assert
    EXPECT_TRUE(result_err.find("content") != std::string::npos);
};


// Test case: Returns a valid JSON format of response from the agent server.
TEST_F(AgencyRequestWrapperTest, ValidJSON) {
    EXPECT_CALL(mock_agency_request_wrapper1, get_linux_distro())
        .WillOnce(Return(distro));
    EXPECT_CALL(mock_agency_request_wrapper1, get_installed_packages())
        .WillOnce(Return(packages));
    EXPECT_CALL(mock_agency_request_wrapper1, get_ssh_ip())
        .WillOnce(Return(ssh_ip));
    EXPECT_CALL(mock_agency_request_wrapper1, get_ssh_port())
        .WillOnce(Return(ssh_port));
    EXPECT_CALL(mock_agency_request_wrapper1, get_ssh_user())
        .WillOnce(Return(ssh_user));

    json response_body = {
        {"content", "Test JSON"}
    };

    json response = {
        {"status_code", "200"},
        {"headers", {}},
        {"body", response_body}
    };

    EXPECT_CALL(mock_agency_request_wrapper1, make_http_request(_, _, _, _, _))
        .WillOnce(Return(response));

    // Call
    json result = mock_agency_request_wrapper1.send_request_to_agent_server(url, query, session_history);

    EXPECT_TRUE(result.contains("body") && result["body"].contains("content") && result["body"]["content"] == "Test JSON");
};