#include <stddef.h>
#define _GNU_SOURCE            // memmem

#include <stdio.h>             // printf, fprintf, snprintf
#include <stdlib.h>            // exit, malloc, free, strtoull
#include <string.h>            // memmem, memchr, memset, strlen, strncasecmp
#include <strings.h>           // strncasecmp
#include <stdint.h>            // uint16_t
#include <errno.h>             // errno, EINTR, EINPROGRESS, ECONNREFUSED, EMFILE, ERANGE
#include <unistd.h>            // close
#include <fcntl.h>             // fcntl, O_NONBLOCK, F_GETFL, F_SETFL
#include <time.h>              // clock_gettime, CLOCK_MONOTONIC
#include <sys/time.h>          // struct timeval (SO_RCVTIMEO)
#include <sys/socket.h>        // socket, connect, send, recv, getsockopt, setsockopt
#include <sys/types.h>         // ssize_t
#include <sys/select.h>        // select, fd_set
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>             // getaddrinfo, freeaddrinfo, gai_strerror

#define WINDOW   256
#define BUF_SIZE 512

typedef struct probe
{
    int send_first;            // 1 — client-first (слать payload), 0 — server-first
    const char *payload;
    const char *pattern;
    const char *service;
} probe_query;

typedef struct probe_port
{
    int fd;
    int port;
    long long deadline;
} probe_port;

typedef enum {
    PORT_OPENED,
    PORT_CLOSED,
    PORT_FILTERED,
    PORT_ERROR
} port_t;

typedef enum {
    PROBE_SUCCESS,
    PROBE_LIMIT,
    PROBE_ERROR
} probe_t;

static volatile sig_atomic_t stop = 0;
static void on_sigint(int sig) { (void)sig; stop = 1; }

const probe_query SSH_PROBE  = { 0, NULL,                      "SSH-",  "ssh"  };
const probe_query HTTP_PROBE = { 1, "HEAD / HTTP/1.0\r\n\r\n", "HTTP/", "http" };
const probe_query SMTP_PROBE = { 0, NULL,                      "220 ",  "smtp" };
const probe_query FTP_PROBE  = { 0, NULL,                      "220 ",  "ftp"  };

int c_opened = 0;
int c_closed = 0;
int c_filtered = 0;
int c_error = 0;

void extract_http_server(const char *buf, ssize_t res, char *out, size_t out_size);
void extract_banner_version(const char *buf, ssize_t res, char *out, size_t out_size);
ssize_t run_probe(int fd, const struct probe *pr, char *buf, size_t buflen);
const probe_query *probes_for_port(uint16_t port, size_t *n);
void identificate_opened_ports(const char *host, long *opened_ports, long timeout);
void discover_opened_ports(const char *host, long port_from, long port_to, long timeout, long *opened_ports);
probe_t start_port_probe(int epfd, struct sockaddr_in *addr, int port, int timeout, probe_port *slots, int i);
int connect_timeout(const char *host, long port, int timeout_ms);
ssize_t find_free_slot(probe_port *slots);
long long now_ms(void);
long get_l(const char *inp);

int main(int argc, char **argv)
{    
    if (argc != 5)
    {
        fprintf(stderr, "usage: %s <host> <port-from> <port-to> <timeout-ms>\n", argv[0]); exit(EXIT_FAILURE);
    }

    long port_from = get_l(argv[2]);
    if (port_from == 0 || port_from > 65535)
    {
        fprintf(stderr, "error: port-from must be >0 & <65536\n"); exit(EXIT_FAILURE);
    }

    long port_to = get_l(argv[3]);
    if (port_to == 0 || port_to > 65535)
    {
        fprintf(stderr, "error: port-to must be >0 & <65536\n"); exit(EXIT_FAILURE);
    }

    if (port_from > port_to)
    {
        fprintf(stderr, "error: port-to < port-from\n"); exit(EXIT_FAILURE);
    }
    long timeout_ms = get_l(argv[4]);

    const char *host = argv[1];

    long *opened_ports = malloc(sizeof(long) * (port_to - port_from + 1));
    if (opened_ports == NULL)
    {
        fprintf(stderr, "malloc error\n"); exit(EXIT_FAILURE);
    }

    discover_opened_ports(host, port_from, port_to, timeout_ms, opened_ports);

    identificate_opened_ports(host, opened_ports, timeout_ms);

    fprintf(stdout, "results: %d opened, %d closed, %d filtered, %d error\n",
          c_opened, c_closed, c_filtered, c_error);

    return 0;
}

void identificate_opened_ports(const char *host, long *opened_ports, long timeout)
{
    for (int i = 0; i < c_opened; i++) // run through the ports (L1)
    {
        long cur_port = opened_ports[i];

        int fd = connect_timeout(host, cur_port, timeout); // created fd
        if (fd == -1)
        {
            fprintf(stderr, "connect_timeout error\n"); continue;
        }

        size_t n = 0; const probe_query *pr = probes_for_port(cur_port, &n); // matches
        if (n == 0) // no matches
        {
            printf("port %ld: open unknown\n", cur_port);
            close(fd); continue;
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
                    if (found != NULL) // pattern
                    {
                        char detail[256];
                        if (strcmp(pr[k].service, "http") == 0)
                        {
                            extract_http_server(buf, res, detail, sizeof(detail));
                        }
                        else
                        {
                            extract_banner_version(buf, res, detail, sizeof(detail));
                        }
                        printf("port %ld: open %s %s\n", cur_port, pr[k].service, detail);
                    }
                }
            }
        }
        close(fd);
    }
}

void extract_http_server(const char *buf, ssize_t res, char *out, size_t out_size)
{
    const char *end = buf + res;
    const char *p = memmem(buf, res, "Server:", 7);
    if (p == NULL)
        return;
    p += 7;
    for ( ; p < end; p++)
    {
        if (*p != ' ')
            break;
    }
    const char *q = memchr(p, '\r', end - p);
    if (q == NULL)
        q = end;

    size_t need = q - p;
    size_t n = need < out_size - 1 ? need : out_size - 1;

    memcpy(out, p, n);
    out[n] = '\0';
}

void extract_banner_version(const char *buf, ssize_t res, char *out, size_t out_size)
{
    const char *end = buf + res;
    const char *banner = memchr(buf, '\r', res);
    if (banner == NULL)
        banner = end;

    size_t need = banner - buf;
    size_t n = need < out_size - 1 ? need : out_size - 1;

    memcpy(out, buf, n);
    out[n] = '\0';
}

ssize_t run_probe(int fd, const probe_query *pr, char *buf, size_t buflen)
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

const probe_query *probes_for_port(uint16_t port, size_t *n)
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

void discover_opened_ports(const char *host, long port_from, long port_to, long timeout, long *opened_ports)
{
    struct sigaction sa;
    sa.sa_handler = on_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    int epfd = epoll_create1(0);
    if (epfd == -1)
    {
        perror("epoll_create1"); exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    if (inet_pton(AF_INET, host, &addr.sin_addr) != 1)
    {
        perror("inet_pton"); exit(EXIT_FAILURE);
    }
    addr.sin_family = AF_INET;

    probe_port slots[WINDOW];
    memset(&slots, -1, sizeof(slots));
    long cur_port = port_from;
    int active = 0;
    while (!stop && (cur_port <= port_to || active > 0))
    {
        int i;
        while (cur_port <= port_to && (i = find_free_slot(slots)) != -1)
        {
            int res = start_port_probe(epfd, &addr, cur_port, timeout, &(slots[i]), i);
            if (res == PROBE_SUCCESS)
            {
                cur_port++;
                active++;
            }
            else if (res == PROBE_ERROR)
            {
                c_error++;
                cur_port++;
            }
            else if (res == PROBE_LIMIT)
            {
                break;
            }
        }

        long long t = now_ms();
        for (int k = 0; k < WINDOW; k++)
        {
            if (slots[k].fd != -1 && slots[k].deadline < t)
            {
                close(slots[k].fd);
                slots[k].fd = -1;
                c_filtered++;
                active--;
            }
        }

        long long epoll_timeout_ms = t + timeout;
        for (int k = 0; k < WINDOW; k++)
            if (slots[k].fd != -1 && slots[k].deadline < epoll_timeout_ms)
                epoll_timeout_ms = slots[k].deadline;
        epoll_timeout_ms -= now_ms();
        epoll_timeout_ms = epoll_timeout_ms < 0 ? 0: epoll_timeout_ms;

        struct epoll_event evs[WINDOW];
        int n = epoll_wait(epfd, evs, WINDOW, epoll_timeout_ms);
        if (n > 0)
        {
            for (int j = 0; j < n; j++)
            {
                int k = evs[j].data.u32; // индекс для slots
                int soerr; socklen_t len = sizeof(soerr);
                getsockopt(slots[k].fd, SOL_SOCKET, SO_ERROR, &soerr, &len);
                if (soerr == 0)
                {
                    opened_ports[c_opened++] = slots[k].port;
                }
                else if (soerr == ECONNREFUSED)
                {
                    c_closed++;
                }
                else
                {
                    c_error++;
                }
                close(slots[k].fd);
                slots[k].fd = -1;
                active--;
            }
        }
        else if (n == -1)
        {
            if (errno == EINTR)
                continue;
            else
            {
                perror("epoll_wait"); stop = 1; continue;
            }
        }
    } 
}

int connect_timeout(const char *host, long port, int timeout_ms)
{
    struct addrinfo hints, *info;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; hints.ai_socktype = SOCK_STREAM;

    char portstr[6];
    snprintf(portstr, sizeof(portstr), "%ld", port);
    int rv = getaddrinfo(host, portstr, &hints, &info);
    if (rv != 0)
    {
        fprintf(stderr, "port %ld: %s\n", port, gai_strerror(rv)); return -1;
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
            if (n == 0)
            {
                close(fd); continue;
            }
            else if (n > 0) 
            {
                int soerr; socklen_t len = sizeof(soerr);
                getsockopt(fd, SOL_SOCKET, SO_ERROR, &soerr, &len);
                if (soerr == 0)
                    return fd;
                close(fd); continue;
            }
            else if (n < 0)
            {
                close(fd); continue;
            }
        }
        else if (res == -1 && errno != EINPROGRESS)
        {
            close(fd); continue;
        }
    }

    return -1;
}

probe_t start_port_probe(int epfd, struct sockaddr_in *addr, int port, int timeout, probe_port *slots, int i)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        if (errno == EMFILE)
            return PROBE_LIMIT;
        perror("socket"); exit(EXIT_FAILURE);
    }

    int flags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in target = *addr;
    target.sin_port = htons(port);

    int res = connect(fd, (struct sockaddr *)&target, sizeof(target));
    if (res == 0 || (res == -1 && (errno == EINPROGRESS || errno == ECONNREFUSED)))
    {
        struct epoll_event ev;
        ev.events  = EPOLLOUT;   
        ev.data.u32 = i;
        epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    }
    else
    {
        perror("connect"); close(fd); return PROBE_ERROR;
    }

    slots->fd = fd;
    slots->port = port;
    slots->deadline = now_ms() + (long long)timeout;
    return PROBE_SUCCESS;
}

ssize_t find_free_slot(probe_port *slots)
{
    for (int i = 0; i < WINDOW; i++)
        if (slots[i].fd == -1)
            return i;
    return -1;
}

long long now_ms(void)
{
      struct timespec ts;
      clock_gettime(CLOCK_MONOTONIC, &ts);
      return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

long get_l(const char *inp)
{
    char *endptr;
    errno = 0;
    long value = strtol(inp, &endptr, 10);
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
