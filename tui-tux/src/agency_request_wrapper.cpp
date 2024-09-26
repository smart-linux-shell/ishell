#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <sys/utsname.h>
#include <string>
#include <iostream>
#include "../nlohmann/json.hpp"
#include "../include/https_client.hpp"
#include "../include/agency_request_wrapper.hpp"

#define INSTALLED_PACKAGES_BUFSIZ 128

using json = nlohmann::json;

// Function to get Linux distribution
std::string AgencyRequestWrapper::get_linux_distro() {
    struct utsname buffer;
    if (uname(&buffer) != 0) {
        return "Unknown";
    }
    return buffer.sysname; // or use buffer.release, buffer.version
}

// Function to get installed packages (simple example using dpkg on Debian/Ubuntu systems)
std::vector<std::string> AgencyRequestWrapper::get_installed_packages() {
    std::vector<std::string> packages;
    FILE* pipe = popen("dpkg --get-selections | awk '{print $1}'", "r");
    if (!pipe) {
        return packages;
    }
    char buffer[INSTALLED_PACKAGES_BUFSIZ];
    while (fgets(buffer, 128, pipe) != nullptr) {
        packages.push_back(buffer);
    }
    pclose(pipe);
    return packages;
}

// Function to get SSH IP, port, and user from environment variables (? might change ?)
std::string AgencyRequestWrapper::get_ssh_ip() {
    char* ssh_ip = getenv("SSH_CLIENT");
    if (ssh_ip) {
        std::string ssh_ip_str(ssh_ip);
        return ssh_ip_str.substr(0, ssh_ip_str.find(' '));
    }
    return "Unknown";
}

int AgencyRequestWrapper::get_ssh_port() {
    char* ssh_port = getenv("SSH_PORT");
    if (ssh_port) {
        return std::stoi(ssh_port);
    }
    return 22;
}

std::string AgencyRequestWrapper::get_ssh_user() {
    char* ssh_user = getenv("USER");
    if (ssh_user) {
        return std::string(ssh_user);
    }
    return "Unknown";
}

// Function to send request to agent's server
json AgencyRequestWrapper::send_request_to_agent_server(const std::string& url, const std::string& user_query) {
    std::string distro = get_linux_distro();
    std::vector<std::string> installed_packages = get_installed_packages();
    std::string ssh_ip = get_ssh_ip();
    int ssh_port = get_ssh_port();
    std::string ssh_user = get_ssh_user();

    json request_body = {
        {"distro", distro},
        {"installed_packages", installed_packages},
        {"query", user_query},
        {"ssh_ip", ssh_ip},
        {"ssh_port", ssh_port},
        {"ssh_user", ssh_user}
    };

    std::map<std::string, std::string> headers = {
        {"Content-Type", "application/json"}
    };

    json response = make_http_request(HttpRequestType::POST, url, {}, request_body, headers);

    return response;
}

// Wrapper prep function for request for agent
std::string AgencyRequestWrapper::ask_agent(const std::string& url, const std::string& user_query) {
    json response = send_request_to_agent_server(url, user_query);

    if (response.contains("error")) {
        std::cerr << "Request failed: " << response["error"] << std::endl;
        return "";
    }

    json response_body = response["body"];

    if (response_body.contains("content")) {
        return response_body["content"].get<std::string>();
    } else {
        std::cerr << "\"content\" field not found in response body" << std::endl;
        return "";
    }
}

json AgencyRequestWrapper::make_http_request(HttpRequestType request_type, const std::string& url,
                       const std::map<std::string, std::string>& query_params = {},
                       const json& body = nullptr,
                       const std::map<std::string, std::string>& headers = {}) {
    HttpsClient https_client;
    return https_client.make_http_request(request_type, url, query_params, body, headers);
}

char *AgencyRequestWrapper::getenv(const char *key) {
    return std::getenv(key);
}