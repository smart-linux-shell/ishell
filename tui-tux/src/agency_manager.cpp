#include <cstdlib>
#include <iostream>
#include "../include/agency_manager.hpp"

AgencyRequestWrapper agency_request_wrapper;

AgencyManager::AgencyManager() : agency_url_set(false) {
    get_agency_url();
}

AgencyManager::~AgencyManager() {}

void AgencyManager::get_agency_url() {
    agency_url_env = getenv("ISHELL_AGENCY_URL");
    if (agency_url_env != NULL) {
        agency_url_set = true;
        agency_url = agency_url_env;
        assistant_url = agency_url + "/assistant";
    }
}

std::string AgencyManager::execute_query(const std::string &query, std::vector<std::pair<std::string, std::string>> &session_history) {
    if (!agency_url_set) {
        std::cout << "ISHELL_AGENCY_URL not set\n";
        return "";
    }
    std::string result = agency_request_wrapper.ask_agent(assistant_url, query);
    session_history.push_back({query, result});
    return result;
}
