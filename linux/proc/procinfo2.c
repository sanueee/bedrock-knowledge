#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>

typedef struct {
    char pid[21];
    uint64_t vmrss_kb;
    char  name[64];
} ProcInfo;

void ProcessByPID(const char *arg);
void ProcessTop();
int cmp_proc(const void *a, const void *b);

int main(int argc, char **argv)
{
    if (argc == 2)
    {
        ProcessByPID(argv[1]);
    }
    else if (argc < 2)
    {
        ProcessTop();
    }

    return 0;
}

void ProcessByPID(const char *arg)
{
    char *endptr;
    errno = 0;
    uint64_t pid = strtoull(arg, &endptr, 10);
    if (arg == endptr)
    {
        fprintf(stderr, "error: int required\n");
        return;
    }
    if (errno == ERANGE)
    {
        fprintf(stderr, "error: int overflow\n");
        return;
    }

    char pid_string[21];
    snprintf(pid_string, sizeof(pid_string), "%" PRIu64, pid);

    char path[64] = "/proc/";
    strncat(path, pid_string, 21);
    strncat(path, "/status", 7);

    FILE* inp = fopen(path, "r");
    if (inp == NULL)
    {
        fprintf(stderr, "error: opening %s\n", path);
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
                fprintf(stdout, "Memory:\t%" PRIu64 " MB\n", res_mb);
            }
        }
    }
    fclose(inp);
}

void ProcessTop()
{
    DIR *proc_dir = opendir("/proc/");
    if (proc_dir == NULL)
    {
        fprintf(stderr, "error: opening /proc/\n");
        return;
    }

    struct dirent *curr_dir = NULL;
    ProcInfo proc_arr[1024]; int count = 0;
    while ((curr_dir = readdir(proc_dir)) != NULL)
    {
        char *endptr;
        strtoull(curr_dir->d_name, &endptr, 10);
        if (*endptr == '\0'  &&  endptr != curr_dir->d_name)
        {
            char curr_path[256];
            snprintf(curr_path, sizeof(curr_path), "/proc/%s/status", curr_dir->d_name);
            FILE *status_file = fopen(curr_path, "r");
            if (status_file == NULL)
            {
                fprintf(stderr, "error: opening %s\n", curr_path);
                continue;
            }
            
            char line[1024];
            ProcInfo curr_proc = {0};
            while (fgets(line, sizeof(line), status_file) != NULL)
            {
                if (strncmp(line, "Name:", 5) == 0)
                {
                    char name[64];
                    if (sscanf(line + 5, "%63s", name) == 1)
                    {
                        memcpy(curr_proc.name, name, 64);
                    }
                }
                else if (strncmp(line, "Pid:", 4) == 0)
                {
                    char pid_val[21];
                    if (sscanf(line + 4, "%20s", pid_val) == 1)
                    {
                        memcpy(curr_proc.pid, pid_val, 21);
                    }
                }
                else if (strncmp(line, "VmRSS:", 6) == 0)
                {
                    char val[64], units[16];
                    if (sscanf(line + 6, "%63s %15s", val, units) == 2)
                    {
                        uint64_t res_kb = strtoull(val, NULL, 10);
                        //uint64_t res_mb = res_kb / 1024;
                        curr_proc.vmrss_kb = res_kb;
                    }
                }
            }
            proc_arr[count] = curr_proc;
            count++;
            fclose(status_file);
        }
    }
    closedir(proc_dir);


    qsort(proc_arr, count, sizeof(ProcInfo), cmp_proc);
    fprintf(stdout, "Top 5 most memory process:");
    for (int i = 0; i < 5 && i < count; i++)
    {
        fprintf(stdout, "Name: %s\n", proc_arr[i].name);
        fprintf(stdout, "PID: %s\n", proc_arr[i].pid);
        fprintf(stdout, "Memory: %" PRIu64 " kb\n", proc_arr[i].vmrss_kb);
        fprintf(stdout, "\n");
    }
}

int cmp_proc(const void *a, const void *b)
{
    const ProcInfo *pa = (const ProcInfo *)a;
    const ProcInfo *pb = (const ProcInfo *)b;
    if (pa->vmrss_kb == pb->vmrss_kb) return 0;
    return (pa->vmrss_kb > pb->vmrss_kb) ? -1 : 1;
}