#ifndef HTTPS_CLIENT_HPP
#define HTTPS_CLIENT_HPP

#include <string>
#include <map>
#include <curl/curl.h>
#include "../nlohmann/json.hpp"

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

json make_http_request(HttpRequestType request_type, const std::string& url,
                       const std::map<std::string, std::string>& query_params = {},
                       const json& body = nullptr,
                       const std::map<std::string, std::string>& headers = {});

std::string build_query_string(const std::map<std::string, std::string>& query_params);
void set_request_type(CURL* curl, HttpRequestType request_type);
void add_request_body(CURL* curl, HttpRequestType request_type, const json& body, std::string& jsonData);
void add_request_headers(CURL* curl, const std::map<std::string, std::string>& headers);
void set_response_callbacks(CURL* curl, std::string& readBuffer, std::map<std::string, std::string>& response_headers);
json perform_request(CURL* curl);

#endif // HTTPS_CLIENT_HPP
