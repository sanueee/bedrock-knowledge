#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

static volatile sig_atomic_t count = 0;

void handler(int sig)
{
    count++;
    const char msg[] = "got SIGINT\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        fprintf(stderr, "./... --mode <signal/sigaction>\n");
        return 1;
    }

    if (strcmp(argv[2], "signal") == 0)
    {
        if (signal(SIGINT, handler) == SIG_ERR) { perror("signal"); return 1; }
    }
    else if (strcmp(argv[2], "sigaction") == 0)
    {
        struct sigaction sa = {0};
        sa.sa_handler = handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;
        if (sigaction(SIGINT, &sa, NULL) == -1) { perror("sigaction"); return 1; }
    }
    else { fprintf(stderr, "error: wrong mode\n"); return 1; }

    sig_atomic_t last_seen = 0;
    while (1) {
        pause();
        while (last_seen < count) {
            last_seen++;
            printf("count=%d\n", last_seen);
        }
    }

    return 0;
}