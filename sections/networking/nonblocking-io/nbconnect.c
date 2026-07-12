#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <poll.h>


int main(int argc, char **argv)
{
    if (argc < 3 || argc > 4)
    {
        fprintf(stderr, "usage: %s <host> <port> [timeout_ms]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *host = argv[1];
    char *service = argv[2];
    uint64_t timeout_ms = 3000;
    if (argc == 4)
    {
        char *endptr;
        errno = 0;
        timeout_ms = strtoull(argv[3], &endptr, 10);
        if (argv[3] == endptr)
        {
            fprintf(stderr, "error: int required\n");
            exit(EXIT_FAILURE);
        }
        if (errno == ERANGE)
        {
            fprintf(stderr, "error: int overflow\n");
            exit(EXIT_FAILURE);
        }
    }

    struct addrinfo hints, *info;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int rv = getaddrinfo(host, service, &hints, &info);
    if (rv != 0) {
        fprintf(stderr, "%s\n", gai_strerror(rv));
        exit(EXIT_FAILURE);
    }
    
    int fd; int found = 0;
    for (struct addrinfo *p = info; p != NULL; p = p->ai_next)
    {
        fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd == -1)
        {
            perror("socket");
            continue;
        }
        int flags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);

        int res = connect(fd, p->ai_addr, p->ai_addrlen);
        if (res == 0)
        {
            found = 1;
            break;
        }
        else if (res != 0)
        {
            if (errno == EINPROGRESS)
            {
                struct pollfd pfd = { .fd = fd, .events = POLLOUT };
                int n;
                do {
                    n = poll(&pfd, 1, timeout_ms);
                } while (n == -1 && errno == EINTR);

                if (n == 0)  { close(fd); continue; }

                int so_err = 0;
                socklen_t len = sizeof(so_err);
                if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &so_err, &len) == -1) {
                    perror("getsockopt");
                    close(fd);
                    continue;
                }
                if (so_err == 0)
                {
                    found = 1;
                    break;
                }
                else if (so_err != 0) {
                    fprintf(stderr, "connect: %s\n", strerror(so_err));
                    close(fd);
                    continue;
                }
            }
            else { close(fd); continue; }
        }
    }
    if (found)
    {
        char buf[512];
        struct pollfd pfd = { .fd = fd, .events = POLLIN };
        int n;
        do {
            n = poll(&pfd, 1, timeout_ms);
        } while (n == -1 && errno == EINTR);

        for (;;) {
            ssize_t r = recv(fd, buf, sizeof(buf) - 1, 0);
            if (r > 0)
            {
                buf[r] = '\0';
                fprintf(stdout, "%s\n", buf);
            }
            else if (r == 0) break;
            else if (errno == EINTR) continue;
            else if (errno == EAGAIN) break;
            else { perror("recv"); break; }
        }
        close(fd);
    }

    freeaddrinfo(info);
    exit(EXIT_SUCCESS);
}