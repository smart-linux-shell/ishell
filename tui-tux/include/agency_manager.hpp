#ifndef AGENCY_HPP
#define AGENCY_HPP

#include <string>
#include <vector>
#include <agency_request_wrapper.hpp>

class AgencyManager {
public:
    explicit AgencyManager(AgencyRequestWrapper* request_wrapper);
    virtual ~AgencyManager();

    std::string get_agency_url();
    virtual std::string execute_query(const std::string &endpoint, const std::string &query);

    std::string agency_url;
    bool agency_url_set = false;

    AgencyRequestWrapper* request_wrapper;

    // <query, result>
    std::vector<std::pair<std::string, std::string>> session_history;
};

#endif // AGENCY_HPP
