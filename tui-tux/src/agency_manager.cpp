#include <cstdlib>
#include <iostream>
#include <agency_manager.hpp>

AgencyManager::AgencyManager(AgencyRequestWrapper* request_wrapper)
    : agency_url_set(false), request_wrapper(request_wrapper) {
    get_agency_url();
}

AgencyManager::~AgencyManager() {}

void AgencyManager::get_agency_url() {
    agency_url_env = getenv("ISHELL_AGENCY_URL");
    if (agency_url_env != nullptr) {
        agency_url_set = true;
        agency_url = agency_url_env;
        assistant_url = agency_url + "/assistant";
    }
}

std::string AgencyManager::execute_query(const std::string &query, std::vector<std::pair<std::string, std::string>> &session_history) {
    if (!agency_url_set) {
        std::cerr << "ISHELL_AGENCY_URL not set\n";
        return "";
    }
    std::string result = request_wrapper->ask_agent(assistant_url, query);
    session_history.push_back({query, result});
    return result;
}
