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

    [[nodiscard]] virtual std::string get_agent_name() const;
    virtual void set_agent_name(const std::string &agent_name);

    std::string agency_url;
    bool agency_url_set = false;
    std::string agent_name = "assistant";

    AgencyRequestWrapper* request_wrapper;

    // <query, result>
    std::vector<std::pair<std::string, std::string>> session_history;
};

#endif // AGENCY_HPP
