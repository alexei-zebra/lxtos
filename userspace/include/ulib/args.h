#include <libc/stdio.h>
#include <libc/string.h>


char *optarg = NULL;
int   optind = 1;
int   opterr = 1;
int   optopt = 0;

static char *optcursor = NULL;
static int  nextchar   = 0;


int getopt(int argc, char * const argv[], const char *optstring)
{
    /* Init: if optcursor == NULL, start new token */
    if (optcursor == NULL) {
        nextchar = 0;
    }

    if (optind >= argc || argv[optind] == NULL) {
        return -1;
    }

    /* If the option cluster is exhausted or not yet set */
    if (optcursor == NULL || *optcursor == '\0') {
        if (strcmp(argv[optind], "--") == 0) {
            optind++;
            optcursor = NULL;
            return -1;
        }
        if (argv[optind][0] != '-' || argv[optind][1] == '\0') {
            optcursor = NULL;
            return -1;
        }
        /* New option cluster */
        optcursor = &argv[optind][1];
        nextchar = 0;
    }

    optopt = optcursor[nextchar];
    if (optopt == '\0') {
        /* Empty cluster (should not happen) */
        optind++;
        optcursor = NULL;
        return -1;
    }

    const char *pos = strchr(optstring, optopt);
    if (pos == NULL) {
        if (opterr)
                printf("%s: illegal option -- %c\n", argv[0], optopt);
        nextchar++;
        if (optcursor[nextchar] == '\0') {
            optind++;
            optcursor = NULL;
        }
        return '?';
    }

    /* Option with argument */
    if (pos[1] == ':') {
        /* Check for attached argument */
        if (optcursor[nextchar + 1] != '\0') {
                if (opterr)
                        printf("%s: option '-%c' cannot have attached argument\n",
                               argv[0], optopt);
            optind++;
            optcursor = NULL;
            return '?';
        }
        /* Separate argument */
        if (optind + 1 < argc) {
            optarg = argv[++optind];
            optind++;                   /* Move to the token after the argument */
            optcursor = NULL;
            return optopt;
        } else {
            if (opterr)
                    printf("%s: option requires an argument -- %c\n",
                           argv[0], optopt);
            optind++;
            optcursor = NULL;
            return '?';
        }
    } else {
        /* Option without argument */
        nextchar++;
        if (optcursor[nextchar] == '\0') {
            /* Cluster finished */
            optind++;
            optcursor = NULL;
        }
        optarg = NULL;
        return optopt;
    }
}
