#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/epoll.h>

typedef enum {
    PROBE_OPEN,
    PROBE_CLOSED,
    PROBE_FILTERED,
    PROBE_ERROR
} probe_result_t;

probe_result_t probe_port(const char *ip, int port, int timeout_ms)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) { perror("socket"); exit(EXIT_FAILURE); }

    int flags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1)
    {
        fprintf(stderr, "error ip: %s\n", ip);
        exit(EXIT_FAILURE);
    }
    addr.sin_port = htons(port);

    int res = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    if (res == 0) { close(fd); return PROBE_OPEN; } 
    else if (res == -1 && errno == EINPROGRESS)
    {
        int epfd = epoll_create1(0);

        struct epoll_event ev;
        ev.events  = EPOLLOUT;   
        ev.data.fd = fd;
        epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
        struct epoll_event out;

        int n;
        while ((n = epoll_wait(epfd, &out, 1, timeout_ms)) == -1 && errno == EINTR);
        if (n == 0) { close(fd); close(epfd); return PROBE_FILTERED; }
        else if (n > 0) {close(epfd); }
        else { perror("epoll_wait"); close(fd); close(epfd); return PROBE_ERROR; }
    }
    else if (res == -1 && errno != EINPROGRESS) { close(fd); return PROBE_ERROR; }
    int soerr; socklen_t len = sizeof(soerr);
    getsockopt(fd, SOL_SOCKET, SO_ERROR, &soerr, &len);
    close(fd);
    if (soerr == 0) return PROBE_OPEN;
    else if (soerr == ECONNREFUSED) return PROBE_CLOSED;
    else return PROBE_ERROR;
}

typedef unsigned long long ull;

ull get_ull(char *inp)
{
    char *endptr;
    errno = 0;
    ull value = strtoull(inp, &endptr, 10);
    if (inp == endptr)
    {
        fprintf(stderr, "error: %s. int required.\n", inp);
        exit(EXIT_FAILURE);
    }
    if (errno == ERANGE)
    {
        fprintf(stderr, "error: %s. int overflow\n", inp);
        exit(EXIT_FAILURE);
    }
    return value;
}

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        fprintf(stderr, "usage: %s <ip> <port> <timeout_ms>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    ull port = get_ull(argv[2]);
    if (port > 65535 || port == 0) { fprintf(stderr, "error: port overflow\n"); exit(EXIT_FAILURE); }
    ull timeout_ms = get_ull(argv[3]);

    probe_result_t res = probe_port(argv[1], port, timeout_ms);
    switch (res)
    {
        case PROBE_CLOSED:
            printf("result: closed\n");
            break;
        case PROBE_OPEN:
            printf("result: opened\n");
            break;
        case PROBE_FILTERED:
            printf("result: filtered\n");
            break;
        case PROBE_ERROR:
            printf("result: error\n");
            break;
    }

    exit(EXIT_SUCCESS);
}