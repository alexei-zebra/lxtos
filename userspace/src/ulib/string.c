#include <ulib/string.h>


int u_strlen(const char *s)
{
    int n = 0;
    while (s[n])
            n++;
    return n;
}

int u_strcmp(const char *a, const char *b)
{
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

int u_strncmp(const char *a, const char *b, int n)
{
    for (int i = 0; i < n; i++) {
        if (a[i] != b[i])
                return (unsigned char)a[i] - (unsigned char)b[i];
        if (!a[i])
                return 0;
    }
    return 0;
}

void u_strcpy(char *dst, const char *src, int max)
{
    int i = 0;
    while (i < max - 1 && src[i]) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = 0;
}
