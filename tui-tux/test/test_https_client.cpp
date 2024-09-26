#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../include/https_client.hpp"

#include <iostream>

using namespace testing;

class MockHttpsClient1 : public HttpsClient {
public:
    MOCK_METHOD(CURL *, curl_easy_init, (), (override));
};

class HttpsClientTest : public Test {
public:
    HttpsClient https_client;
    MockHttpsClient1 mock_https_client1;

    std::map<std::string, std::string> headers;

    void SetUp() override {
        headers["Content-Type"] = "application/json";
    }
};

// Test case: Successfully makes a GET request with no query parameters.
TEST_F(HttpsClientTest, GetNoQueryParams) {
    json response = https_client.make_http_request(HttpRequestType::GET, "https://reqres.in/api/users/2", {}, {}, headers);

    EXPECT_TRUE(response.contains("body") && response.contains("status_code") && response["status_code"] == 200 && response["body"].contains("data"));
}

// Test case: Successfully makes a POST request with a JSON body and headers.
TEST_F(HttpsClientTest, PostJson) {
    json request = {
        {"email", "eve.holt@reqres.in"},
        {"password", "pistol"}
    };

    json response = https_client.make_http_request(HttpRequestType::POST, "https://reqres.in/api/register", {}, request, headers);

    EXPECT_TRUE(response.contains("body") && response.contains("status_code") && response["status_code"] == 200 && response["body"].contains("id") && response["body"].contains("token"));
};

// Test case: Successfully makes a PUT request with a JSON body.
TEST_F(HttpsClientTest, PutJson) {
    json request = {
        {"name", "morpheus"},
        {"job", "zion resident"}
    };

    json response = https_client.make_http_request(HttpRequestType::PUT, "https://reqres.in/api/users/2", {}, request, headers);

    EXPECT_TRUE(response.contains("body") && response.contains("status_code") && response["status_code"] == 200 && response["body"].contains("name") && response["body"].contains("job"));
};

// Test case: Successfully makes a DELETE request.
TEST_F(HttpsClientTest, Delete) {
    json response = https_client.make_http_request(HttpRequestType::DELETE, "https://reqres.in/api/users/2", {}, {}, {});

    EXPECT_TRUE(response.contains("status_code") && response["status_code"] == 204);
};

// Test case: Returns error when CURL initialization fails.
TEST_F(HttpsClientTest, InitFail) {
    EXPECT_CALL(mock_https_client1, curl_easy_init)
        .WillOnce(Return(nullptr));

    json response = mock_https_client1.make_http_request(HttpRequestType::GET, "https://example.com", {}, {}, {});

    EXPECT_TRUE(response.contains("error"));
};

// Test case: Handles query parameters correctly in the URL.
TEST_F(HttpsClientTest, GetQueryParams) {
    std::map<std::string, std::string> query_params;
    query_params["page"] = "2";

    json response = https_client.make_http_request(HttpRequestType::GET, "https://reqres.in/api/users", query_params, {}, headers);

    EXPECT_TRUE(response.contains("body") && response.contains("status_code") && response["status_code"] == 200 && response["body"].contains("page") && response["body"]["page"] == 2);
};
