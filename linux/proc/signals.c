#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

static FILE *log_file;

void handler(int sig)
{
    if (sig == SIGINT)
    {
        fprintf(log_file, "[%u] caught SIGINT\n", getpid());
        return;
    }
    else if (sig == SIGTERM)
    {
        fprintf(log_file, "[%u] caught SIGTERM, shutting down\n", getpid());
        exit(0);
    }
}

int main(void)
{
    log_file = fopen("signals.log", "w");
    fprintf(log_file, "[%u] started\n", getpid());

    struct sigaction sa;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    while (1)
    {
        pause();
    }
    
    return 0;
}