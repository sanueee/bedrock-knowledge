#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

void ProcessByPID(const char *arg);

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "error: argument required");
        return 1;
    }
    ProcessByPID(argv[1]);
    return 0;
}

void ProcessByPID(const char *arg)
{
    char *endptr;
    errno = 0;
    uint64_t pid = strtoull(arg, &endptr, 10);
    if (arg == endptr)
    {
        fprintf(stderr, "error: int required");
        return;
    }
    if (errno == ERANGE)
    {
        fprintf(stderr, "error: int overflow");
        return;
    }

    char pid_string[21];
    snprintf(pid_string, sizeof(pid_string), "%llu", pid);

    char path[64] = "/proc/";
    strncat(path, pid_string, 21);
    strncat(path, "/status", 7);

    FILE* inp = fopen(path, "r");
    if (inp == NULL)
    {
        fprintf(stderr, "error: opening %s", path);
        return;
    }
    
    char line[128];
    while (fgets(line, sizeof(line), inp))
    {
        if (strncmp(line, "Name:", 5) == 0) {
            char name[64];
            if (sscanf(line + 5, "%63s", name) == 1) {
                fprintf(stdout, "Process:\t%s\n", name);
            }
        }
        else if (strncmp(line, "Pid:", 4) == 0) {
            char pid_val[21];
            if (sscanf(line + 4, "%20s", pid_val) == 1) {
                fprintf(stdout, "PID:\t%s\n", pid_val);
            }
        }
        else if (strncmp(line, "VmRSS:", 6) == 0) {
            char val[64], units[16];
            if (sscanf(line + 6, "%63s %15s", val, units) == 2) {
                uint64_t res_kb = strtoull(val, NULL, 10);
                uint64_t res_mb = res_kb / 1024;
                fprintf(stdout, "Memory:\t%llu MB\n", res_mb);
            }
        }
    }
    fclose(inp);
}