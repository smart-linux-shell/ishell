#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

void get_agency_url();
std::string execute_query(const std::string &query, std::vector<std::pair<std::string, std::string>> &session_history);