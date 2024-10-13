#ifndef AGENCY_HPP
#define AGENCY_HPP

#include <string>
#include <vector>
#include <agency_request_wrapper.hpp>

class AgencyManager {
public:
    explicit AgencyManager(AgencyRequestWrapper* request_wrapper);
    virtual ~AgencyManager();

    void get_agency_url();
    virtual std::string execute_query(const std::string &query);

    bool agency_url_set;
    std::string agency_url;
    std::string assistant_url;
    const char* agency_url_env{};
    AgencyRequestWrapper* request_wrapper;

    // <query, result>
    std::vector<std::pair<std::string, std::string>> session_history;
};

#endif // AGENCY_HPP
