#ifndef HTTPS_CLIENT_H
#define HTTPS_CLIENT_H

#include <string>
#include <map>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

// Enum class to define HTTP request types
enum class HttpRequestType {
    GET,
    POST,
    PUT,
    DELETE,
    HEAD,
    OPTIONS,
    PATCH
};

// Function to make an HTTP request
// Parameters:
// - request_type: Type of the HTTP request (GET, POST, PUT, DELETE, HEAD, OPTIONS, PATCH)
// - url: The URL for the request
// - query_params: Optional map of query parameters (default is empty)
// - body: Optional JSON body for POST, PUT, PATCH requests (default is nullptr)
// - headers: Optional map of headers (default is empty)
// Returns: JSON object with the response data
json make_http_request(HttpRequestType request_type,
                       const std::string& url,
                       const std::map<std::string, std::string>& query_params = {},
                       const json& body = nullptr,
                       const std::map<std::string, std::string>& headers = {});

#endif // HTTPS_CLIENT_H
