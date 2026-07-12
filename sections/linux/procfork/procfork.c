#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "error: N required");
        return 1;
    }

    char *end; errno = 0;
    long N = strtol(argv[1], &end, 10);
    if (errno != 0 || end == argv[1] || *end != '\0' || N <= 0)
    {
        fprintf(stderr, "error: arguent must be integer > 0");
        return 1;
    }
    
    pid_t pids[N];
    for (int i = 0; i < N; i++)
    {
        pids[i] = fork();
        if (pids[i] == -1) { fprintf(stderr, "error: fork() for %d", i); }
        if (pids[i] == 0)
        {
            fprintf(stdout, "[child %d] pid=%d ppid=%d\n", i, getpid(), getppid());
            exit(i);
        }
    }

    for (int i = 0; i < N; i++)
    {
        int status;
        waitpid(pids[i], &status, 0);
        fprintf(stdout, "[parent] child %d exited with code %d\n", pids[i], WEXITSTATUS(status));
    }

    return 0;
}