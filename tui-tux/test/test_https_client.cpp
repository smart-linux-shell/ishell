#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <https_client.hpp>

#include <iostream>

using namespace testing;

class MockHttpsClient1 final : public HttpsClient {
public:
    MOCK_METHOD(CURL *, curl_easy_init, (), (override));
};

class MockHttpsClient2 final : public HttpsClient {
public:
    MOCK_METHOD(void, set_response_callbacks, (CURL* curl, std::string& readBuffer, (std::map<std::string, std::string>&) response_headers), (override));
    MOCK_METHOD(CURLcode, curl_easy_perform, (CURL *curl), (override));
};

class MockHttpsClient3 final : public HttpsClient {
public:
    MOCK_METHOD(CURLcode, curl_easy_perform, (CURL *curl), (override));
};

class HttpsClientTest : public Test {
public:
    HttpsClient https_client;
    MockHttpsClient1 mock_https_client1;
    MockHttpsClient2 mock_https_client2;
    MockHttpsClient3 mock_https_client3;

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
    const json request = {
        {"email", "eve.holt@reqres.in"},
        {"password", "pistol"}
    };

    json response = https_client.make_http_request(HttpRequestType::POST, "https://reqres.in/api/register", {}, request, headers);

    EXPECT_TRUE(response.contains("body") && response.contains("status_code") && response["status_code"] == 200 && response["body"].contains("id") && response["body"].contains("token"));
};

// Test case: Successfully makes a PUT request with a JSON body.
TEST_F(HttpsClientTest, PutJson) {
    const json request = {
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

    const json response = mock_https_client1.make_http_request(HttpRequestType::GET, "https://example.com", {}, {}, {});

    EXPECT_TRUE(response.contains("error"));
};

// Test case: Handles query parameters correctly in the URL.
TEST_F(HttpsClientTest, GetQueryParams) {
    std::map<std::string, std::string> query_params;
    query_params["page"] = "2";

    json response = https_client.make_http_request(HttpRequestType::GET, "https://reqres.in/api/users", query_params, {}, headers);

    EXPECT_TRUE(response.contains("body") && response.contains("status_code") && response["status_code"] == 200 && response["body"].contains("page") && response["body"]["page"] == 2);
};

// Test case: Correctly handles a non-JSON response body.
TEST_F(HttpsClientTest, HandleNonJson) {
    std::string *buf;

    EXPECT_CALL(mock_https_client2, set_response_callbacks(_, _, _))
        .WillOnce(Invoke([&](CURL* curl, std::string& readBuffer, std::map<std::string, std::string>& response_headers) {
            buf = &readBuffer;
        }));
    
    EXPECT_CALL(mock_https_client2, curl_easy_perform(_))
        .WillOnce(Invoke([&](CURL *curl) -> CURLcode {
            *buf = "Test No JSON";

            return CURLE_OK;
        }));
    
    // Call
    
    CURL *curl = curl_easy_init();
    json response = mock_https_client2.perform_request(curl);

    EXPECT_TRUE(response.contains("body") && response["body"] == "Test No JSON");
};

// Test case: Returns an error message if the request fails.
TEST_F(HttpsClientTest, HandleError) {
    EXPECT_CALL(mock_https_client3, curl_easy_perform(_))
        .WillOnce(Invoke([](CURL *curl) -> CURLcode {
            return CURLE_AGAIN;
        }));

    // Call
    CURL *curl = curl_easy_init();
    json response = mock_https_client3.perform_request(curl);

    EXPECT_TRUE(response.contains("error") && response["error"] == "Socket not ready for send/recv");
}

// Test case: Correctly retrieves response headers and status code.
TEST_F(HttpsClientTest, HeadersAndStatusCode1) {
    const std::string url = "https://reqres.in/api/users?page=2";
    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    json response = https_client.perform_request(curl);

    EXPECT_TRUE(response.contains("status_code") && response["status_code"] == 200);
    EXPECT_TRUE(response.contains("headers") && response["headers"].contains("content-type") && response["headers"]["content-type"].dump().find("application/json") != std::string::npos);
}

TEST_F(HttpsClientTest, HeadersAndStatusCode2) {
    const std::string url = "https://reqres.in/api/users/23";
    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    json response = https_client.perform_request(curl);

    EXPECT_TRUE(response.contains("status_code") && response["status_code"] == 404);
    EXPECT_TRUE(response.contains("headers") && response["headers"].contains("content-type") && response["headers"]["content-type"].dump().find("application/json") != std::string::npos);
}