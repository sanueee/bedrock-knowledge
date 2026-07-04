#include <stddef.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/epoll.h>

#define WINDOW 256

typedef unsigned long long ull;

static int c_opened = 0;
static int c_closed = 0;
static int c_filtered = 0;
static int c_error = 0;

struct probe {
    int fd;
    int port;
    long long deadline_ms;
};

typedef enum {
    SUCCESS,
    SOCKET_LIMIT,
    CONNECT_ERROR
} func_probe_result_t;

typedef enum {
    PROBE_OPEN,
    PROBE_CLOSED,
    PROBE_FILTERED,
    PROBE_ERROR
} probe_result_t;

static volatile sig_atomic_t stop = 0;
static void on_sigint(int sig) { (void)sig; stop = 1; }

static func_probe_result_t start_probe(int epfd, const struct sockaddr_in *base, int port, int timeout_ms, struct probe *slot, int slot_idx);
static ssize_t find_free_slot(struct probe *slot);
static long long now_ms(void);
static ull get_ull(const char *inp);
static void print_probe_result(int port, int cond);

int main(int argc, char **argv) {
    
    if (argc != 5) {
        fprintf(stderr, "usage: %s <ip> <port-from> <port-to> <timeout-ms>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    ull port_from = get_ull(argv[2]);
    if (port_from == 0 || port_from > 65535)
    {
        fprintf(stderr, "error: port-from must be >0 & <65536\n");
        exit(EXIT_FAILURE);
    }
    ull port_to = get_ull(argv[3]);
    if (port_to == 0 || port_to > 65535)
    {
        fprintf(stderr, "error: port-to must be >0 & <65536\n");
        exit(EXIT_FAILURE);
    }
    if (port_from > port_to)
    {
        fprintf(stderr, "error: port-to < port-from\n");
        exit(EXIT_FAILURE);
    }
    ull timeout_ms = get_ull(argv[4]);
    struct sockaddr_in base;
    memset(&base, 0, sizeof(base));
    if (inet_pton(AF_INET, argv[1], &base.sin_addr) != 1) {
        perror("inet_pton"); exit(EXIT_FAILURE);
    }
    base.sin_family = AF_INET;

    int epfd = epoll_create1(0);
    if (epfd == -1) { perror("epoll_create1"); exit(EXIT_FAILURE); }

    struct probe slots[WINDOW];
    memset(&slots, -1, sizeof(slots));

    struct sigaction sa;
    sa.sa_handler = on_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    
    int next_port = (int)port_from;
    int active = 0;
    while (!stop && (next_port <= (int)port_to || active > 0)) {
        int i;
        while (next_port <= (int)port_to && (i = find_free_slot(slots)) != -1)
        {
            int res = start_probe(epfd, &base, next_port, timeout_ms, &(slots[i]), i);
            if (res == SUCCESS) { // удачно закинули в epfd
                next_port++;
                active++;
            }
            else if (res == SOCKET_LIMIT) { // нет места для создания сокета - ждем
                break;
            }
            else if (res == CONNECT_ERROR) { // ошибка подключения - пропускаем
                print_probe_result(next_port, PROBE_ERROR);
                c_error++;
                next_port++;
            }
        }

        // дедлайн просрочен -> filtered
        long long t = now_ms();
        for (int k = 0; k < WINDOW; k++)
        {
            if (slots[k].fd != -1 && slots[k].deadline_ms < t)
            {
                print_probe_result(slots[k].port, PROBE_FILTERED);
                close(slots[k].fd);
                slots[k].fd = -1;
                c_filtered++;
                active--;
            }
        }

        long long epoll_timeout_ms = t + timeout_ms;
        for (int k = 0; k < WINDOW; k++)
            if (slots[k].fd != -1 && slots[k].deadline_ms < epoll_timeout_ms)
                epoll_timeout_ms = slots[k].deadline_ms;
        epoll_timeout_ms -= now_ms();
        epoll_timeout_ms = epoll_timeout_ms < 0 ? 0: epoll_timeout_ms;

        struct epoll_event evs[WINDOW];
        int n = epoll_wait(epfd, evs, WINDOW, epoll_timeout_ms);

        if (n > 0) {
            for (int j = 0; j < n; j++)
            {
                int k = evs[j].data.u32; // индекс для slots
                int soerr; socklen_t len = sizeof(soerr);
                getsockopt(slots[k].fd, SOL_SOCKET, SO_ERROR, &soerr, &len);
                if (soerr == 0)
                {
                    print_probe_result(slots[k].port, PROBE_OPEN);
                    c_opened++;
                }
                else if (soerr == ECONNREFUSED)
                {
                    print_probe_result(slots[k].port, PROBE_CLOSED);
                    c_closed++;
                }
                else {
                    print_probe_result(slots[k].port, PROBE_ERROR);
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
            else { perror("epoll_wait"); stop = 1; continue; }
        }
    }

    printf("results: %d opened, %d closed, %d filtered, %d error\n",
          c_opened, c_closed, c_filtered, c_error);

    return 0;
}

static func_probe_result_t start_probe(int epfd, const struct sockaddr_in *base,
                       int port, int timeout_ms, struct probe *slot, int slot_idx)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        if (errno == EMFILE) {
            return SOCKET_LIMIT;
        }
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int flags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in target = *base;
    target.sin_port = htons(port);

    int res = connect(fd, (struct sockaddr *)&target, sizeof(target));
    if (res == 0
         || (res == -1 && errno == EINPROGRESS)
         || (res == -1 && errno == ECONNREFUSED)) {
        struct epoll_event ev;
        ev.events  = EPOLLOUT;   
        ev.data.u32 = slot_idx;
        epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    }
    else { perror("connect"); close(fd); return CONNECT_ERROR; }

    slot->fd = fd;
    slot->port = port;
    slot->deadline_ms = now_ms() + timeout_ms;
    return SUCCESS;
}

static void print_probe_result(int port, int cond)
{
    switch (cond) {
        case PROBE_FILTERED:
            printf("port %d: filtered\n", port);
            break;
        case PROBE_CLOSED:
            printf("port %d: closed\n", port);
            break;
        case PROBE_OPEN:
            printf("port %d: opened\n", port);
            break;
        case PROBE_ERROR:
            printf("port %d: error\n", port);
            break;
    }
}

static long long now_ms(void)
{
      struct timespec ts;
      clock_gettime(CLOCK_MONOTONIC, &ts);
      return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static ull get_ull(const char *inp)
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

static ssize_t find_free_slot(struct probe *slot)
{
    for (int i = 0; i < WINDOW; i++)
        if (slot[i].fd == -1)
            return i;
    return -1;
}