#ifndef AGENCY_REQUEST_WRAPPER_HPP
#define AGENCY_REQUEST_WRAPPER_HPP

#include <string>
#include <vector>
#include "../nlohmann/json.hpp"
#include <https_client.hpp>

using json = nlohmann::json;

class AgencyRequestWrapper {
public:
    virtual std::string get_linux_distro();
    virtual std::vector<std::string> get_installed_packages();
    virtual std::string get_ssh_ip();
    virtual int get_ssh_port();
    virtual std::string get_ssh_user();
    json send_request_to_agent_server(const std::string &url, const std::string &user_query);
    virtual std::string ask_agent(const std::string &url, const std::string &user_query);
    virtual json make_http_request(HttpRequestType request_type, const std::string& url,
                        const std::map<std::string, std::string>& query_params,
                        const json& body,
                        const std::map<std::string, std::string>& headers);
    virtual char *getenv(const char *key);
};

#endif // AGENCY_REQUEST_WRAPPER_HPP
