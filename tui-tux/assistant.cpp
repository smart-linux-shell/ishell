#include <readline/readline.h>
#include <readline/history.h>
#include <stdio.h>
#include <stdlib.h>

#include <iostream>

#include "agency_request_wrapper.hpp"

void assistant() {
    using_history();

    while (1) {
        char *input = readline("assistant> ");

        if (!input) {
            break;
        }

        add_history(input);

        std::string result = ask_agent("http://127.0.0.1:5000/agents/assistant", std::string(input));

        std::cout << result << "\n";

        free(input);
    }
}