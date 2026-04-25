#pragma once
#include <ulib/string.h>
#include <ulib/syscall.h>

static inline void u_resolve_path(const char *input, char *result, int max)
{
    if (input[0] == '/') {
        u_strcpy(result, input, max);
        return;
    }
    sys_getcwd(result, max);
    int len = u_strlen(result);
    if (len > 1 && result[len-1] != '/') result[len++] = '/', result[len] = 0;
    for (int i = 0; input[i] && len < max-1; i++)
        result[len++] = input[i];
    result[len] = 0;
}