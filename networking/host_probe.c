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

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *host = argv[1];
    char *service = argv[2];

    struct addrinfo hints, *info;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    int rv = getaddrinfo(host, service, &hints, &info);
    if (rv != 0)
    {
        fprintf(stderr, "%s\n", gai_strerror(rv));
        exit(EXIT_FAILURE);
    }

    struct addrinfo *p; int fd;
    for (p = info; p != NULL; p = p->ai_next)
    {
        fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd == -1)
        {
            perror("socket");
            continue;
        }

        if (connect(fd, p->ai_addr, p->ai_addrlen) == -1)
        {
            perror("connect");
            close(fd);
            continue;
        }
        
        if (p->ai_family == AF_INET)
        {
            char buf[INET_ADDRSTRLEN];
            struct sockaddr_in *v4 = (struct sockaddr_in *)p->ai_addr;
            if (inet_ntop(v4->sin_family, &(v4->sin_addr), buf, sizeof(buf)) == NULL)
                perror("inet_ntop");
            uint16_t port = ntohs(v4->sin_port);
            fprintf(stdout, "connected: %s:%u\n", buf, port);
        }
        else if (p->ai_family == AF_INET6)
        {
            char buf[INET6_ADDRSTRLEN];
            struct sockaddr_in6 *v6 = (struct sockaddr_in6 *)p->ai_addr;
            if (inet_ntop(v6->sin6_family, &(v6->sin6_addr), buf, sizeof(buf)) == NULL)
                perror("inet_ntop");
            uint16_t port = ntohs(v6->sin6_port);
            fprintf(stdout, "connected: %s:%u\n", buf, port);
        }
        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "host_probe: can't connect to %s:%s\n", argv[1], argv[2]);
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(info);

    struct timeval tv = { .tv_sec = 3, .tv_usec = 0 };
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    char buf[256];
    ssize_t n;
    do {
        n = recv(fd, buf, sizeof(buf) - 1, 0);
    } while (n == -1 && errno == EINTR);

    if (n > 0)
    {
        buf[n] = '\0';
        fprintf(stdout, "banner: %s\n", buf);
    }
    else if (n == 0)
        fprintf(stdout, "connection closed without banner\n");
    else {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            fprintf(stderr, "server didn't send banner (timeout)\n");
        else
            perror("recv");
    }

    close(fd);
    exit(EXIT_SUCCESS);
}