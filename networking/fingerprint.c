#define _GNU_SOURCE

#define BUF_SIZE 512

#include <stdio.h>       // printf, fprintf, snprintf
#include <stdlib.h>      // exit, strtol
#include <string.h>      // memmem, memchr, memset, strlen
#include <stdint.h>      // uint16_t
#include <errno.h>       // errno, EINTR, EAGAIN, EWOULDBLOCK, EINPROGRESS
#include <unistd.h>      // close
#include <fcntl.h>       // fcntl, O_NONBLOCK
#include <sys/time.h>    // struct timeval
#include <sys/socket.h>  // socket, connect, send, recv, setsockopt, SO_RCVTIMEO
#include <sys/types.h>   // ssize_t
#include <sys/select.h>  // select, fd_set  (если connect-таймаут через select)
#include <netdb.h>       // getaddrinfo, freeaddrinfo, gai_strerror

typedef struct probe {
    int send_first;   // 1 — client-first (слать payload), 0 — server-first
    const char *payload;
    const char *pattern;
    const char *service;
} probe;

static const probe SSH_PROBE  = { 0, NULL,                      "SSH-",  "ssh"  };
static const probe HTTP_PROBE = { 1, "HEAD / HTTP/1.0\r\n\r\n", "HTTP/", "http" };
static const probe SMTP_PROBE = { 0, NULL,                      "220 ",  "smtp" };
static const probe FTP_PROBE  = { 0, NULL,                      "220 ",  "ftp"  };

long chtol(const char *inp);
int connect_timeout(const char *host, uint16_t port, int timeout_ms);
ssize_t run_probe(int fd, const struct probe *pr, char *buf, size_t buflen);
static const probe *probes_for_port(uint16_t port, size_t *n);

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "usage: %s <host> <port> [port ...]", argv[0]); exit(EXIT_FAILURE);
    }

    char *host = argv[1];
    for (int i = 2; i < argc; i++) // run through the ports (L1)
    {
        char *str_port = argv[i];
        long port = chtol(str_port);
        if (port == -1)
            continue;

        int fd = connect_timeout(host, port, 3000); // created fd
        if (fd == -1)
        {
            fprintf(stderr, "connect_timeout error\n"); continue;
        }

        size_t n; const probe *pr = probes_for_port(port, &n); // matches
        if (n == 0) // no matches
        {
            fprintf(stdout, "port %ld: unknown\n", port);  close(fd); continue;
        }
        else // matches
        {
            for (size_t k = 0; k < n; k++) // run through the matches (L2)
            {
                char buf[BUF_SIZE] = {0};
                ssize_t res = run_probe(fd, &pr[k], buf, BUF_SIZE);
                if (res > 0) // smth in buf else continue
                {
                    char *found = memmem(buf, res, pr[k].pattern, strlen(pr[k].pattern));
                    if (found == NULL) // no pattern
                    {
                        fprintf(stdout, "port %ld: no match\n", port); continue;
                    }
                    fprintf(stdout, "port %ld: service - %s, banner - %.*s\n", port, pr[k].service, 
                        (int)res, buf);
                }
            }
        }
        close(fd);
    }
}

ssize_t run_probe(int fd, const struct probe *pr, char *buf, size_t buflen)
{
    int flags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
    struct timeval tv = { .tv_sec = 3, .tv_usec = 0 };
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    if (pr->send_first)
    {
        size_t len = strlen(pr->payload);
        ssize_t sent = 0; size_t total = 0;
        while (total < len)
        {
            sent = send(fd, pr->payload + total, len - total, 0);
            if (sent == -1 && errno == EINTR) continue;
            else if (sent == -1) return -1;
            total += sent;
        }
    }

    ssize_t rv;
    while ((rv = recv(fd, buf, buflen, 0)) == -1 && errno == EINTR);
    if (rv > 0)
        return rv;
    else
        return -1;
}

int connect_timeout(const char *host, uint16_t port, int timeout_ms)
{
    struct addrinfo hints, *info;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; hints.ai_socktype = SOCK_STREAM;

    char portstr[6];
    snprintf(portstr, sizeof(portstr), "%u", port);
    int rv = getaddrinfo(host, portstr, &hints, &info);
    if (rv != 0)
    {
        fprintf(stderr, "port %u: %s\n", port, gai_strerror(rv)); return -1;
    }

    for (struct addrinfo *p = info; p != NULL; p = p->ai_next)
    {
        int fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd == -1)
        {
            perror("socket"); continue;
        }

        int flags = fcntl(fd, F_GETFL);
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);

        int res = connect(fd, p->ai_addr, p->ai_addrlen);
        if (res == 0)
            return fd;
        else if (res == -1 && errno == EINPROGRESS)
        {
            fd_set wfds; FD_ZERO(&wfds); FD_SET(fd, &wfds);
            struct timeval tv = { .tv_sec = timeout_ms / 1000,
                            .tv_usec = (timeout_ms % 1000) * 1000 };
            int n;
            while ((n = select(fd + 1, NULL, &wfds, NULL, &tv)) == -1 && errno == EINTR);
            if (n == 0) { close(fd); continue; }
            else if (n > 0) 
            {
                int soerr; socklen_t len = sizeof(soerr);
                getsockopt(fd, SOL_SOCKET, SO_ERROR, &soerr, &len);
                if (soerr == 0)
                    return fd;
                close(fd); continue;
            }
            else if (n < 0) { close(fd); continue; }
        }
        else if (res == -1 && errno != EINPROGRESS) { close(fd); continue; }
        
    }

    return -1;
}

const probe *probes_for_port(uint16_t port, size_t *n)
{
    switch (port)
    {
        case 22:                     *n = 1; return &SSH_PROBE;
        case 80: case 8080: case 8443: *n = 1; return &HTTP_PROBE;
        case 25: case 587:           *n = 1; return &SMTP_PROBE;
        case 21:                     *n = 1; return &FTP_PROBE;
        default:                     *n = 0; return NULL;
    }
}

long chtol(const char *inp)
{
    char *endptr;
    errno = 0;
    long value = strtol(inp, &endptr, 10);
    if (inp == endptr)
    {
        fprintf(stderr, "error: %s. int required.\n", inp);
        return -1;
    }
    if (errno == ERANGE)
    {
        fprintf(stderr, "error: %s. int overflow\n", inp);
        return -1;
    }
    if (value < 1 || value > 65535)
    {
        fprintf(stderr, "error: %s: port must be [1 .. 65535]\n", inp);
        return -1;
    }
    return value;
}
