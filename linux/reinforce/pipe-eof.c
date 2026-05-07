#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>

int main(void)
{
    int fd[2];
    if (pipe(fd) == -1) { perror("pipe"); exit(1); }

    pid_t p = fork();
    if (p == 0)
    {
        close(fd[0]);
        char *buf = "hello";
        write(fd[1], buf, strlen(buf));
        exit(0);
    }
    else if (p > 0) 
    {
        //close(fd[1]);

        char buf[256]; ssize_t n;
        while ((n = read(fd[0], buf, sizeof(buf) - 1)) > 0)
        {
            buf[n] = '\0';
            printf("parent received: %s\n", buf);
        }
        close(fd[0]);
        int status;
        waitpid(p, &status, 0);
    }
    else
    {
        perror("fork");
    }
    return 0;
}