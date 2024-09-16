#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include "assistant_query.hpp"
#include "agency_request_wrapper.hpp"

std::string execute_query(const std::string &query, std::vector<std::pair<std::string, std::string>> &session_history) {
    std::string result = ask_agent("http://127.0.0.1:5000/agents/assistant", query);
    session_history.push_back({query, result});
    return result;
}
