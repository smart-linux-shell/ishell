#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <sys/utsname.h>
#include <string>
#include <iostream>
#include <fstream>
#include "../nlohmann/json.hpp"
#include <https_client.hpp>
#include <agency_request_wrapper.hpp>
#include <session_tracker.hpp>

#define INSTALLED_PACKAGES_BUFSIZ 128

using json = nlohmann::json;

// Function to get Linux distribution
std::string AgencyRequestWrapper::get_linux_distro() {
    std::ifstream f("/etc/os-release");
    if (f) {
        std::string line;
        while (std::getline(f, line)) {
            if (line.rfind("ID=", 0) == 0) {
                std::string id = line.substr(3);
                if (!id.empty() && id.front() == '"')  id.erase(0, 1);
                if (!id.empty() && id.back()  == '"')  id.pop_back();
                return id;            // arch, debian, ubuntu, fedora, …
            }
        }
    }
    // fallback → uname
    utsname buf{};
    return (uname(&buf) == 0) ? std::string(buf.sysname) : "Unknown";
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
        packages.emplace_back(buffer);
    }
    pclose(pipe);
    return packages;
}

// Function to get SSH IP, port, and user from environment variables (? might change ?)
std::string AgencyRequestWrapper::get_ssh_ip() {
    if (const char* ssh_ip = getenv("SSH_IP")) {
        std::string ssh_ip_str(ssh_ip);
        return ssh_ip_str.substr(0, ssh_ip_str.find(' '));
    }
    return "Unknown";
}

int AgencyRequestWrapper::get_ssh_port() {
    if (const char* ssh_port = getenv("SSH_PORT")) {
        return std::stoi(ssh_port);
    }
    return 22;
}

std::string AgencyRequestWrapper::get_ssh_user() {
    if (char* ssh_user = getenv("USER")) {
        std::string ssh_user_str = std::string(ssh_user);
        return ssh_user_str;
    }
    return "Unknown";
}

// Function to send request to agent's server
json AgencyRequestWrapper::send_request_to_agent_server(const std::string& url, std::string user_query, bool include_session_history) {
    std::string distro = get_linux_distro();
    std::vector<std::string> installed_packages = get_installed_packages();
    std::string ssh_ip = get_ssh_ip();
    int ssh_port = get_ssh_port();
    std::string ssh_user = get_ssh_user();
	std::string history = "";
	if(include_session_history){
    	std::string history = get_session_history_string();
	}

    if(context.size() > 0){
        user_query += "\n<additional context>\n";
        for(auto doc : context){
            user_query += doc + "\n";
        }
    }

    const json request_body = {
        {"distro", distro},
        {"installed_packages", installed_packages},
        {"query", user_query},
        {"session_history", history},
        {"ssh_ip", ssh_ip},
        {"ssh_port", ssh_port},
        {"ssh_user", ssh_user}
    };

    const char *token_env = getenv("ISHELL_TOKEN");

    std::map<std::string, std::string> headers = {
        {"Content-Type", "application/json"},
    };

    if (token_env != nullptr) {
        headers.emplace("Authorization", std::string("token ") + std::string(token_env));
    }

    json response = make_http_request(HttpRequestType::POST, url, {}, request_body, headers);

    return response;
}

std::string AgencyRequestWrapper::get_session_history_string() {
    const auto& hist = SessionTracker::get().get_history(); // vector<Interaction>

    std::ostringstream buf;

    for (unsigned int i = 0; i < hist.size(); i++)
    {
        const auto& inter = hist[i];
        buf << '[' << inter.timestamp << "] Question: " << inter.question << '\n'
            << "Answer: " << inter.answer << "\n\n";

        for (const auto& cmd : inter.shell)
        {
            buf << '[' << cmd.execution_start << "] " << cmd.command << '\n'
                << cmd.output << '\n'
                << "[exit code: " << cmd.exit_code
                << ", finished " << cmd.execution_end << "]\n\n";
        }
        buf << "----------------------------------------\n";
    }
    return buf.str();
}

// Wrapper prep function for request for agent
std::string AgencyRequestWrapper::ask_agent(const std::string& url, std::string user_query, bool include_session_history) {
    json response = send_request_to_agent_server(url, user_query, include_session_history);

    const json response_body = response["body"];

    if (response_body.contains("error")) {
//        std::cerr << "Request failed: " << response_body["error"] << std::endl;
        return "";
    }

    if (response_body.contains("content")) {
        return response_body["content"].get<std::string>();
    }

    std::cerr << "\"content\" field not found in response body" << std::endl;
    std::cerr << response.dump(2) << '\n';
    return "";
}

json AgencyRequestWrapper::make_http_request(const HttpRequestType request_type, const std::string& url,
                                             const std::map<std::string, std::string>& query_params,
                                             const json& body,
                                             const std::map<std::string, std::string>& headers) {
    HttpsClient https_client;
    return https_client.make_http_request(request_type, url, query_params, body, headers);
}

char *AgencyRequestWrapper::getenv(const char *key) {
    return std::getenv(key);
}