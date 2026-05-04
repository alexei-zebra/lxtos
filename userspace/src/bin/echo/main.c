#include <libc/stdlib.h>
#include <libc/stdio.h>


void main(int argc, char **argv)
{
    printf("");
    for (int i = 1; i < argc; i++) {
        printf("%s", argv[i]);
        if (i != argc-1)
                printf(" ");
    }
    printf("");

    exit(0);
}
