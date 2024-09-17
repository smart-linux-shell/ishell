#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include "../include/assistant_query.hpp"
#include "../include/agency_request_wrapper.hpp"


std::string agency_url, assistant_url;
char *agency_url_env;
bool agency_url_set;

void get_agency_url() {
    agency_url_env = getenv("ISHELL_AGENCY_URL");
    if (agency_url_env != NULL) {
        agency_url_set = true;
        agency_url = agency_url_env;
        assistant_url = agency_url + "/assistant";
    }
}

std::string execute_query(const std::string &query, std::vector<std::pair<std::string, std::string>> &session_history) {
    if (!agency_url_set) {
        std::cout << "ISHELL_AGENCY_URL not set\n";
        return "";
    }
    std::string result = ask_agent(assistant_url, query);
    session_history.push_back({query, result});
    return result;
}
