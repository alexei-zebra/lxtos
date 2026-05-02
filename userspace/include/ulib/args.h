#pragma once

#include <stdbool.h>
#include <libc/string.h>

#define MAX_ARG_LEN 255

char arg[MAX_ARG_LEN];

int get_opt(const int argc, const char **argv, const char opt, const bool b_with_arg)
{
    // set argument count

    int cnt = argc;
    for (int i = 1; i < argc; i++)
        if (argv[i][0] == '-' && argv[i][1] == '-' && strlen(argv[i]) == 2) // argument is end separator
                cnt = i;


    // find option

    for (int i = 1; i < cnt; i++) {
        if (argv[i][0] == '-' && argv[i][1] != '-') // is a option
            if (strlen(argv[i]) > 2) { // is a multi-option
                for (char *ops = argv[i] + 1; *ops; ops++) {
                    if (*ops == opt) { // is a matching option
                        if (b_with_arg)
                                return -1; // option requires an argument but it be in multi-option
                        return 1; // option found
                    }
                }
            } else if (strlen(argv[i]) == 2 && argv[i][1] == opt) { // is a single option and is a matching option
                if (b_with_arg) // option requires an argument
                    if (i + 1 < cnt) {
                        int len = strlen(argv[i + 1]);
                        strncpy(arg, argv[i + 1], len);
                    } else {
                        return -1; // option requires an argument but none provided
                    }
                return 1; // option found
            }
    }
    return 0; // option not found
}