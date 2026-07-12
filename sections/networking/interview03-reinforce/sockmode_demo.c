#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

void on_alarm(int signo)
{
    (void)signo;   // не используем, глушим варнинг
    /* пусто: единственная цель — прервать спящий recv */
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *end; errno = 0;
    long port = strtol(argv[1], &end, 10);
    if (errno != 0 || end == argv[1] || *end != '\0' || port > 65535 || port < 1)
    {
        fprintf(stderr, "error: argument must be port 1 ... 65535\n");
        exit(EXIT_FAILURE);
    }

    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr); 
    if (bind(server, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server, 8) == -1)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    } 

    char test_buf[64];

    int blocking_with_timeout = socket(AF_INET, SOCK_STREAM, 0);
    struct timeval tv = { .tv_sec = 1, .tv_usec = 0 };
    setsockopt(blocking_with_timeout, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    if (connect(blocking_with_timeout, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("connect");
        close(blocking_with_timeout);
    }
    struct timespec time_start, time_end;
    clock_gettime(CLOCK_MONOTONIC, &time_start);
    recv(blocking_with_timeout, test_buf, sizeof(test_buf), 0);
    clock_gettime(CLOCK_MONOTONIC, &time_end);
    fprintf(stdout, "blocking +timeout 1s: %ld, error: %s\n", \
        (time_end.tv_sec-time_start.tv_sec)*1000 + (time_end.tv_nsec - time_start.tv_nsec)/1000000, \
        strerror(errno));
    close(blocking_with_timeout);

    int non_blocking = socket(AF_INET, SOCK_STREAM, 0);
    int flags = fcntl(non_blocking, F_GETFL, 0);
    fcntl(non_blocking, F_SETFL, flags | O_NONBLOCK);
    if (connect(non_blocking, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        if (errno != EINPROGRESS)
        {
            perror("connect");
            close(non_blocking);
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &time_start);
    recv(non_blocking, test_buf, sizeof(test_buf), 0);
    clock_gettime(CLOCK_MONOTONIC, &time_end);
    fprintf(stdout, "non-blocking: %ld, error: %s\n", \
        (time_end.tv_sec-time_start.tv_sec)*1000 + (time_end.tv_nsec - time_start.tv_nsec)/1000000, \
        strerror(errno));
    close(non_blocking);

    int blocking = socket(AF_INET, SOCK_STREAM, 0);
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_alarm;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, NULL);
    if (connect(blocking, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("connect");
        close(blocking);
    }
    alarm(2);
    clock_gettime(CLOCK_MONOTONIC, &time_start);
    recv(blocking, test_buf, sizeof(test_buf), 0);
    clock_gettime(CLOCK_MONOTONIC, &time_end);
    fprintf(stdout, "blocking: %ld, error: %s\n", \
        (time_end.tv_sec-time_start.tv_sec)*1000 + (time_end.tv_nsec - time_start.tv_nsec)/1000000, \
        strerror(errno));
    close(blocking);

    close(server);
    exit(EXIT_SUCCESS);
}