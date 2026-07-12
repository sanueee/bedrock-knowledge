#include <stdio.h>
#include <pthread.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>

typedef struct
{
    int pid;
    char name[256];
    long rss;
} ProcInfo;

typedef struct
{
    int *pids;
    int count;
} ThreadArg;

ProcInfo results[1024];
int results_count = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int read_proc_status(int pid, ProcInfo *out)
{
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    
    FILE *f = fopen(path, "r");
    if (f == NULL) return -1;
    
    char line[256];
    while (fgets(line, sizeof(line), f))
    {
        if (strncmp(line, "Name:", 5) == 0)
            sscanf(line, "Name: %s", out->name);
        else if (strncmp(line, "VmRSS:", 6) == 0)
            sscanf(line, "VmRSS: %ld", &out->rss);
    }
    fclose(f);
    out->pid = pid;
    return 0;
}

void *worker(void *arg)
{
    ThreadArg *t = (ThreadArg *)arg;
    for (int i = 0; i < t->count; i++) {
        ProcInfo info = {0};
        if (read_proc_status(t->pids[i], &info) == 0) 
        {
            pthread_mutex_lock(&mutex);
            results[results_count++] = info;
            pthread_mutex_unlock(&mutex);
        }
    }
    return NULL;
}

int cmp_rss(const void *a, const void *b)
{
    const ProcInfo *pa = (const ProcInfo *)a;
    const ProcInfo *pb = (const ProcInfo *)b;
    return (pb->rss > pa->rss) - (pb->rss < pa->rss);
}

int main(void)
{
    DIR *d_proc = opendir("/proc");
    if (d_proc == NULL)
    {
        fprintf(stderr, "error: opening /proc\n");
        return 1;
    }

    struct dirent *curr_dir = NULL;
    int pid_list[1024]; int count = 0;
    while ((curr_dir = readdir(d_proc)) != NULL)
    {
        char *endptr;
        strtoull(curr_dir->d_name, &endptr, 10);
        if (*endptr == '\0'  &&  endptr != curr_dir->d_name)
        {
            int pid = atoi(curr_dir->d_name);
            if (pid > 0)
            {
                pid_list[count++] = pid;
            }
        }
    }
    closedir(d_proc);

    int chunk = count / 4;

    ThreadArg args[4];
    args[0] = (ThreadArg){ pid_list, chunk };
    args[1] = (ThreadArg){ pid_list + chunk,chunk };
    args[2] = (ThreadArg){ pid_list + chunk * 2,chunk };
    args[3] = (ThreadArg){ pid_list + chunk * 3,count - chunk * 3 }; // остаток

    pthread_t tid[4];
    for (int i = 0; i < 4; i++)
    {
        pthread_create(&tid[i], NULL, worker, &args[i]);
    }

    for (int i = 0; i < 4; i++)
    {
        pthread_join(tid[i], NULL);
    }

    qsort(results, results_count, sizeof(ProcInfo), cmp_rss);
    printf("%-6s %-30s %s\n", "PID", "NAME", "VmRSS (kB)");
    for (int i = 0; i < results_count; i++)
    {
        printf("%-6d %-30s %ld\n", results[i].pid, results[i].name, results[i].rss);
    }
}