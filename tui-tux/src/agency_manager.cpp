#include <cstdlib>
#include <agency_manager.hpp>

#include <utils.hpp>

AgencyManager::AgencyManager(AgencyRequestWrapper* request_wrapper)
    : request_wrapper(request_wrapper) {
}

AgencyManager::~AgencyManager() = default;

std::string AgencyManager::get_agency_url() {
    if (!agency_url_set) {
        if (const char *agency_url_env = getenv("ISHELL_AGENCY_URL"); agency_url_env != nullptr) {
            agency_url = agency_url_env;
        } else {
            agency_url = DEFAULT_AGENCY_URL;
        }
        agency_url_set = true;
    }

    return agency_url;
}

std::string AgencyManager::get_agent_name() const {
    return agent_name;
}

void AgencyManager::set_agent_name(const std::string &agent_name) {
    this->agent_name = agent_name;
}

std::string AgencyManager::execute_query(const std::string &endpoint, std::string query, bool include_session_history) {
    std::string result = request_wrapper->ask_agent(endpoint, query, include_session_history);
    return result;
}
