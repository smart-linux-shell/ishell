#ifndef ASSISTANT_QUERY_HPP
#define ASSISTANT_QUERY_HPP

#include <string>
#include <vector>
#include <utility>

void get_agency_url();
std::string execute_query(const std::string &query, std::vector<std::pair<std::string, std::string>> &session_history);

#endif // ASSISTANT_QUERY_HPP
