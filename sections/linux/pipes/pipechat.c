#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    int fd[2];
    pipe(fd);

    if (fork() == 0) // дочерний
    {
        close(fd[0]);
        char buf[256];
        while (fgets(buf, sizeof(buf), stdin) != NULL)
        {
            write(fd[1], buf, strlen(buf));
        }
        close(fd[1]);
    }
    else
    {
        close(fd[1]);
        char buf[128]; int n;
        while ( (n = read(fd[0], buf, sizeof(buf) - 1)) > 0)
        {
            buf[n] = '\0';
            fprintf(stdout, "[parent received] %s", buf);
        }
        waitpid(-1, NULL, 0);
    }
}