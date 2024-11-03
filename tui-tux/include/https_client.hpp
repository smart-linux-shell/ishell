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

class HttpsClient {
public:
    virtual ~HttpsClient() = default;

    json make_http_request(HttpRequestType request_type, const std::string& url,
                           const std::map<std::string, std::string>& query_params,
                           const json& body,
                           const std::map<std::string, std::string>& headers);

    static std::string build_query_string(const std::map<std::string, std::string>& query_params);
    virtual void set_request_type(CURL* curl, HttpRequestType request_type);
    static void add_request_body(CURL* curl, HttpRequestType request_type, const json& body, std::string& jsonData);
    static void add_request_headers(CURL* curl, const std::map<std::string, std::string>& headers);
    virtual void set_response_callbacks(CURL* curl, std::string& readBuffer, std::map<std::string, std::string>& response_headers);
    virtual json perform_request(CURL* curl);
    virtual CURL *curl_easy_init();
    virtual CURLcode curl_easy_perform(CURL *curl);
};


#endif // HTTPS_CLIENT_HPP
