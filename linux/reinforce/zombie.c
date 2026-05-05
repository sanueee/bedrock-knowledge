#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int main(void)
{
    pid_t p = fork();
    pid_t curr = getpid();
    pid_t child = getppid();
    if (p == 0)
    {
        exit(0);
    }
    printf("curr %d: child - %d\n", curr, child);

    sleep(15);
    int status;
    waitpid(child, &status, 0);
    printf("child %d: status - %d\n", child, status);

    return 0;
}