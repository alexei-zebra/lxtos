#include <ulib/io.h>
#include <ulib/string.h>
#include <ulib/syscall.h>

#define MAX_PATH  256
#define MAX_INPUT 256

static char cwd[MAX_PATH] = "/";
static char hostname[256];

static void resolve_path(const char *input, char *result)
{
    if (input[0] == '/') {
        u_strcpy(result, input, MAX_PATH);
        return;
    }
    u_strcpy(result, cwd, MAX_PATH);
    int len = u_strlen(result);
    if (len > 1 && result[len-1] != '/') result[len++] = '/', result[len] = 0;
    for (int i = 0; input[i] && len < MAX_PATH-1; i++)
        result[len++] = input[i];
    result[len] = 0;
}

static void read_hostname(char *result)
{
    static char buf[256];
    int64_t n = sys_read("/etc/hostname", buf, sizeof(buf) - 1);
    if (n > 0 && buf[n-1] == '\n') buf[n-1] = 0;
    else if (n >= 0) buf[n] = 0;
    u_strcpy(result, buf, 256);
}

static void cmd_pwd(void)
{
    u_puts("\n"); u_puts(cwd); u_puts("\n");
}

static void cmd_cd(const char *arg)
{
    if (!arg) {
        sys_chdir("/");
        sys_getcwd(cwd, MAX_PATH);
        return;
    }
    char path[MAX_PATH];
    resolve_path(arg, path);
    if (sys_chdir(path) == 0)
        sys_getcwd(cwd, MAX_PATH);
}

static void cmd_help(void)
{
    u_puts("\nBuiltins: cd, pwd, echo, clear, exit, help\n");
    u_puts("Other commands run from /bin/<name>\n");
}

static int parse(char *line, char *argv[], int max)
{
    int argc = 0;
    char *p = line;
    while (*p && argc < max) {
        while (*p == ' ') p++;
        if (!*p) break;
        if (*p == '"') {
            p++;
            argv[argc++] = p;
            while (*p && *p != '"') p++;
            if (*p) { *p = 0; p++; }
        } else {
            argv[argc++] = p;
            while (*p && *p != ' ') p++;
            if (*p) { *p = 0; p++; }
        }
    }
    return argc;
}

static void try_exec(const char *name, char **argv, int argc)
{
    char path[MAX_PATH];
    if (name[0] == '/' || (name[0] == '.' && name[1] == '/')) {
        resolve_path(name, path);
    } else {
        u_strcpy(path, "/bin/", MAX_PATH);
        int len = u_strlen(path);
        for (int i = 0; name[i] && len < MAX_PATH-1; i++)
            path[len++] = name[i];
        path[len] = 0;
    }
    if (sys_fsize(path) == (uint64_t)-1) {
        u_puts("\nunknown command: ");
        u_puts(name);
        u_puts("\n");
        return;
    }
    const char *child_argv[16];
    child_argv[0] = path;
    int child_argc = 1;
    for (int i = 1; i < argc && child_argc < 15; i++)
        child_argv[child_argc++] = argv[i];
    child_argv[child_argc] = 0;
    int64_t r = sys_exec(path, child_argv);
    if (r != 0) { u_puts("\nexec failed\n"); }
}

void main(void)
{
    sys_getcwd(cwd, MAX_PATH);
    static char input[MAX_INPUT];
    while (1) {
        read_hostname(hostname);
        u_puts("\n[");
        u_puts(hostname);
        u_puts(":");
        u_puts(cwd);
        u_puts("] >> ");
        u_gets(input, sizeof(input));
        if (!input[0]) continue;
        char *argv[16];
        int argc = parse(input, argv, 16);
        if (argc == 0) continue;
        if      (!u_strcmp(argv[0], "exit"))  { u_puts("\n"); sys_exit(); }
        else if (!u_strcmp(argv[0], "clear")) sys_clear();
        else if (!u_strcmp(argv[0], "help"))  cmd_help();
        else if (!u_strcmp(argv[0], "pwd"))   cmd_pwd();
        else if (!u_strcmp(argv[0], "cd"))    cmd_cd(argc > 1 ? argv[1] : 0);
        else {
            try_exec(argv[0], argv, argc);
        }
    }
}