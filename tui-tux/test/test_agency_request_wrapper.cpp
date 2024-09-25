#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../include/agency_request_wrapper.hpp"
#include "../include/https_client.hpp"

#include <iostream>

using namespace testing;

class MockAgencyRequestWrapper : public AgencyRequestWrapper {
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

class AgencyRequestWrapperTest : public ::testing::Test {
protected:
    MockAgencyRequestWrapper mock_agency_request_wrapper;
};

// Test case: Successfully creates a request with the correct data fields (distro, installed_packages, query, ssh_ip, ssh_port, ssh_user).
TEST_F(AgencyRequestWrapperTest, CorrectRequestData) {
    std::string distro = "Test Distro";
    std::vector<std::string> packages = {"Test Package"};
    std::string ssh_ip = "0.0.0.0";
    int ssh_port = 0;
    std::string ssh_user = "test_user";

    std::string url = "0.0.0.1";
    std::string query = "Test Query";

    EXPECT_CALL(mock_agency_request_wrapper, get_linux_distro())
        .WillOnce(Return(distro));
    EXPECT_CALL(mock_agency_request_wrapper, get_installed_packages())
        .WillOnce(Return(packages));
    EXPECT_CALL(mock_agency_request_wrapper, get_ssh_ip())
        .WillOnce(Return(ssh_ip));
    EXPECT_CALL(mock_agency_request_wrapper, get_ssh_port())
        .WillOnce(Return(ssh_port));
    EXPECT_CALL(mock_agency_request_wrapper, get_ssh_user())
        .WillOnce(Return(ssh_user));

    json request_body = {
        {"distro", distro},
        {"installed_packages", packages},
        {"query", query},
        {"ssh_ip", ssh_ip},
        {"ssh_port", ssh_port},
        {"ssh_user", ssh_user}
    };

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
    
    // Empty
    std::map<std::string, std::string> query_params;
    
    EXPECT_CALL(mock_agency_request_wrapper, make_http_request(HttpRequestType::POST, url, query_params, request_body, headers))
        .WillRepeatedly(Return(response));

    // Call
    mock_agency_request_wrapper.ask_agent(url, query);
};
