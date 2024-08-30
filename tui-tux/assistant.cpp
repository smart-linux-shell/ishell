#include <readline/readline.h>
#include <readline/history.h>
#include <stdio.h>
#include <stdlib.h>

#include <iostream>

void assistant() {
    using_history();

    while (1) {
        char *input = readline("assistant> ");

        if (!input) {
            break;
        }

        add_history(input);

        std::cout << "You wrote: " << input << "\n";

        free(input);
    }
}