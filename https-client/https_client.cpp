#include <iostream>
#include <string>
#include <curl/curl.h>
#include "nlohmann/json.hpp"
#include <sstream>
#include <map>

using json = nlohmann::json;

enum class HttpRequestType {
    GET,
    POST,
    PUT,
    DELETE,
    HEAD,
    OPTIONS,
    PATCH
};

// Helper function to handle the response body
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
    s->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Helper function to capture headers from the response
static size_t HeaderCallback(char* buffer, size_t size, size_t nitems, std::map<std::string, std::string>* headers) {
    std::string header(buffer, size * nitems);
    size_t separator = header.find(':');
    if (separator != std::string::npos) {
        std::string key = header.substr(0, separator);
        std::string value = header.substr(separator + 2, header.length() - separator - 4);  // remove colon and CRLF
        (*headers)[key] = value;
    }
    return nitems * size;
}

// Function to convert query parameters to a URL-encoded string
std::string build_query_string(const std::map<std::string, std::string>& query_params) {
    std::stringstream ss;
    for (const auto& param : query_params) {
        ss << param.first << "=" << param.second << "&";
    }
    std::string query_string = ss.str();
    if (!query_string.empty()) {
        query_string.pop_back(); // remove the trailing '&'
    }
    return query_string;
}

// Function to set CURL options based on the request type
void set_request_type(CURL* curl, HttpRequestType request_type) {
    switch (request_type) {
        case HttpRequestType::POST:
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            break;
        case HttpRequestType::PUT:
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
            break;
        case HttpRequestType::DELETE:
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
            break;
        case HttpRequestType::HEAD:
            curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
            break;
        case HttpRequestType::OPTIONS:
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "OPTIONS");
            break;
        case HttpRequestType::PATCH:
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
            break;
        default:
            break;
    }
}

// Function to add request body
void add_request_body(CURL* curl, HttpRequestType request_type, const json& body, std::string& jsonData) {
    if (request_type == HttpRequestType::POST ||
        request_type == HttpRequestType::PUT ||
        request_type == HttpRequestType::PATCH ||
        request_type == HttpRequestType::DELETE) {

        jsonData = body.dump();
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData.c_str());
    }
}

// Function to add request headers
void add_request_headers(CURL* curl, const std::map<std::string, std::string>& headers) {
    struct curl_slist* curl_headers = nullptr;
    for (const auto& header : headers) {
        std::string header_string = header.first + ": " + header.second;
        curl_headers = curl_slist_append(curl_headers, header_string.c_str());
    }
    if (curl_headers) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);
    }
}

// Function to set CURL options for handling the response
void set_response_callbacks(CURL* curl, std::string& readBuffer, std::map<std::string, std::string>& response_headers) {
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
}

// Function to perform the CURL request and handle the response
json perform_request(CURL* curl) {
    CURLcode res;
    long response_code = 0;
    std::string readBuffer;
    std::map<std::string, std::string> response_headers;

    set_response_callbacks(curl, readBuffer, response_headers);

    res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    json response = {
        {"status_code", response_code},
        {"headers", response_headers},
        {"body", nullptr}
    };

    if (res != CURLE_OK) {
        response["error"] = curl_easy_strerror(res);
    } else {
        try {
            response["body"] = json::parse(readBuffer);
        } catch (json::parse_error&) {
            response["body"] = readBuffer; // if response is not JSON, return raw string
        }
    }

    curl_easy_cleanup(curl);
    return response;
}

// Function to make an HTTP request
json make_http_request(HttpRequestType request_type, const std::string& url,
                       const std::map<std::string, std::string>& query_params = {},
                       const json& body = nullptr,
                       const std::map<std::string, std::string>& headers = {}) {

    CURL* curl = curl_easy_init();
    if (!curl) {
        return json({{"error", "Failed to initialize CURL"}});
    }

    std::string final_url = url;
    if (!query_params.empty()) {
        final_url += "?" + build_query_string(query_params);
    }
    curl_easy_setopt(curl, CURLOPT_URL, final_url.c_str());

    std::string jsonData;

    set_request_type(curl, request_type);
    add_request_body(curl, request_type, body, jsonData);
    add_request_headers(curl, headers);

    // disable SSL verification for simplicity (not recommended for production) !!!
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    return perform_request(curl);
}

int main() {
    std::string url;

    // ex of a GET request with query parameters
    url = "https://httpbin.org/get";
    std::map<std::string, std::string> query_params = {{"test", "123"}, {"foo", "bar"}};
    json get_response = make_http_request(HttpRequestType::GET, url, query_params);
    std::cout << "GET Response: " << get_response.dump(4) << std::endl;

    // ex of a POST request with a JSON body
    url = "https://httpbin.org/post";
    json post_data = {
        {"name", "John Doe"},
        {"age", 30},
        {"email", "johndoe@example.com"}
    };
    json post_response = make_http_request(HttpRequestType::POST, url, {}, post_data, {{"Content-Type", "application/json"}});
    std::cout << "POST Response: " << post_response.dump(4) << std::endl;

    // ex of a DELETE request
    url = "https://httpbin.org/delete";
    json delete_response = make_http_request(HttpRequestType::DELETE, url);
    std::cout << "DELETE Response: " << delete_response.dump(4) << std::endl;

    // ex of a HEAD request
    url = "https://httpbin.org/get";
    json head_response = make_http_request(HttpRequestType::HEAD, url);
    std::cout << "HEAD Response: " << head_response.dump(4) << std::endl;

    // ex of an OPTIONS request
    url = "https://httpbin.org/get";
    json options_response = make_http_request(HttpRequestType::OPTIONS, url);
    std::cout << "OPTIONS Response: " << options_response.dump(4) << std::endl;

    // ex of a PATCH request with a JSON body
    url = "https://httpbin.org/patch";
    json patch_data = {
        {"operation", "update"},
        {"value", "new_value"}
    };
    json patch_response = make_http_request(HttpRequestType::PATCH, url, {}, patch_data, {{"Content-Type", "application/json"}});
    std::cout << "PATCH Response: " << patch_response.dump(4) << std::endl;

    // ex of a GET request with HTTP
    url = "http://httpbin.org/get";
    std::map<std::string, std::string> query_params_http = {{"test", "123"}, {"foo", "bar"}};
    json get_response_http = make_http_request(HttpRequestType::GET, url, query_params_http);
    std::cout << "GET Response (HTTP): " << get_response_http.dump(4) << std::endl;

    return 0;
}
