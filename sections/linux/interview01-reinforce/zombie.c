#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int main(void)
{
    pid_t p = fork();
    if (p < 0) { perror("fork"); exit(1); }
    if (p == 0)
    {
        exit(0);  // child
    }
    printf("curr %d: child - %d\n", getpid(), p);

    sleep(15);
    int status;
    pid_t reaped = waitpid(p, &status, 0);
    if (reaped == -1)
    {
        perror("waitpid");
        exit(1);
    }
    if (WIFEXITED(status))
    {
        printf("child %d exited normally, code = %d\n", p, WEXITSTATUS(status));
    }
    return 0;
}