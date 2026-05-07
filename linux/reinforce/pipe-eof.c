#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

int main(void)
{
    int fd[2];
    pipe(fd);

    pid_t p = fork();
    if (p == 0)
    {
        char *buf = "hello";
        write(fd[1], buf, strlen(buf));
        exit(0);
    }
    else if (p > 0) 
    {
        char *buf; int n;
        while ((n = read(fd[0], buf, sizeof(buf) - 1)) > 0)
        {
            buf[n] = '\0';
            printf("parent received: %s", buf);
        }
    }
    else
    {
        perror("fork: ");
    }
}