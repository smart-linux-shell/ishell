#ifndef AGENCY_HPP
#define AGENCY_HPP

#include <string>
#include <vector>
#include "../include/agency_request_wrapper.hpp"

class AgencyManager {
public:
    AgencyManager();

    void get_agency_url();
    std::string execute_query(const std::string &query, std::vector<std::pair<std::string, std::string>> &session_history);

private:
    std::string agency_url;
    std::string assistant_url;
    char *agency_url_env;
    bool agency_url_set;
};

#endif // AGENCY_HPP
