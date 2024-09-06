#include <readline/readline.h>
#include <readline/history.h>
#include <stdio.h>
#include <stdlib.h>

#include <iostream>

#include "../include/agency_request_wrapper.hpp"

void assistant() {
    using_history();

    std::string agency_url, assistant_url;
    char *agency_url_env;
    bool agency_url_set;

    agency_url_env = getenv("ISHELL_AGENCY_URL");
    if (agency_url_env != NULL) {
        agency_url_set = true;
        agency_url = agency_url_env;
        assistant_url = agency_url + "/assistant";
    }

    while (1) {
        char *input = readline("assistant> ");

        if (!input) {
            break;
        }

        add_history(input);

        if (!agency_url_set) {
            std::cout << "ISHELL_AGENCY_URL not set\n";
            continue;
        }

        std::string result = ask_agent(assistant_url, std::string(input));

        std::cout << result << "\n";

        free(input);
    }
}