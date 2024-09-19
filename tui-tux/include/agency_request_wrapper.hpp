#ifndef AGENCY_REQUEST_WRAPPER_HPP
#define AGENCY_REQUEST_WRAPPER_HPP

#include <string>
#include <vector>
#include "../nlohmann/json.hpp"

using json = nlohmann::json;
std::string get_linux_distro();
std::vector<std::string> get_installed_packages();
std::string get_ssh_ip();
int get_ssh_port();
std::string get_ssh_user();
json send_request_to_agent_server(const std::string &url, const std::string &user_query);
std::string ask_agent(const std::string &url, const std::string &user_query);

#endif // AGENCY_REQUEST_WRAPPER_HPP
