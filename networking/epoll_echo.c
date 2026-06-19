#include <sys/epoll.h>     
#include <sys/socket.h>    
#include <netinet/in.h>    
#include <arpa/inet.h>    
#include <fcntl.h>        
#include <unistd.h>     
#include <signal.h>     
#include <errno.h>      
#include <string.h>     
#include <stdio.h>      
#include <stdlib.h>     
#include <stdint.h>  

#define MAX_EVENTS 64      // размер массива-приёмника для epoll_wait
#define BACKLOG    16      // размер accept queue для listen()
#define BUF_SIZE   4096
#define FD_MAX     4096

typedef struct {
    char   buf[BUF_SIZE];  // байты, ожидающие отправки клиенту
    size_t len;            // сколько всего лежит в buf
    size_t sent;           // сколько из len уже ушло клиенту (offset)
} conn_t;

static conn_t conns[FD_MAX];

// Перевесить fd на новый набор событий (EPOLL_CTL_MOD).
static int arm(int epfd, int fd, uint32_t events)
{
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events  = events;
    ev.data.fd = fd;
    return epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
}

// Дослать накопленный для fd остаток.
// Возврат: 0 — ок (всё ушло ИЛИ ждём EPOLLOUT); -1 — фатальная ошибка сокета.
static int flush_out(int epfd, int fd)
{
    conn_t *c = &conns[fd];
    while (c->sent < c->len)
    {
        ssize_t w = send(fd, c->buf + c->sent, c->len - c->sent, 0);
        if (w > 0) { c->sent += (size_t)w; continue; }          // часть/всё ушло — двигаем offset
        if (w == -1 && errno == EINTR) continue;                // прервал сигнал — повторить тот же send
        if (w == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
            return arm(epfd, fd, EPOLLIN | EPOLLOUT);           // буфер ядра полон → ждать готовности на запись
        return -1;                                              // EPIPE и прочее — соединение мертво
    }
    c->len = c->sent = 0;                                       // backlog пуст
    return arm(epfd, fd, EPOLLIN);                              // EPOLLOUT больше не нужен — снимаем
}

static volatile sig_atomic_t g_stop = 0;

static void on_sigint(int sig)
{
    (void)sig;
    g_stop = 1;
}

static int set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static int make_listen_socket(int port)
{
    int listening = socket(AF_INET, SOCK_STREAM, 0);
    if (listening == -1)
    {
        perror("socket");
        return -1;
    }

    int val = 1;
    if (setsockopt(listening, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) == -1)
    {
        perror("setsockopt");
        close(listening);
        return -1;
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(listening, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("bind");
        close(listening);   // FIX 4: не течь fd на ошибке
        return -1;
    }

    if (listen(listening, BACKLOG) == -1)
    {
        perror("listen");
        close(listening);   // FIX 4: не течь fd на ошибке
        return -1;
    }

    return listening;
}

int main(int argc, char **argv)
{
    long service = 8080;
    if (argc == 2)
    {
        char *end; errno = 0;
        service = strtol(argv[1], &end, 10);
        if (errno != 0 || end == argv[1] || *end != '\0' || service > 65535 || service < 1)
        {
            fprintf(stderr, "error: argument must be int: 1...65535\n");
            exit(EXIT_FAILURE);
        }
    }

    signal(SIGPIPE, SIG_IGN);

    // FIX 1: ставим обработчик SIGINT через sigaction. БЕЗ SA_RESTART —
    // тогда Ctrl+C прервёт epoll_wait с EINTR, цикл проверит g_stop и выйдет
    // на чистую уборку (close listen_fd/epfd) вместо мгновенной гибели процесса.
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    int listen_fd = make_listen_socket((int)service);
    if (listen_fd == -1)
    {
        fprintf(stderr, "make_listen_socket failed\n");
        exit(EXIT_FAILURE);
    }
    if (set_nonblocking(listen_fd) == -1)
    {
        perror("set_nonblocking listen");
        exit(EXIT_FAILURE);
    }

    int epfd = epoll_create1(0);
    if (epfd == -1)
    {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events  = EPOLLIN;
    ev.data.fd = listen_fd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev) == -1)
    {
        perror("epoll_ctl ADD listen");
        exit(EXIT_FAILURE);
    }

    struct epoll_event events[MAX_EVENTS];
    while (!g_stop)
    {
        int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (n == -1)
        {
            if (errno == EINTR)
                continue;

            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < n; i++)
        {
            int fd = events[i].data.fd;
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            if (fd == listen_fd)
            {
                for (;;)
                {
                    int cfd;
                    if ((cfd = accept(fd, (struct sockaddr *)&client_addr, &client_addr_len)) == -1)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break;
                        else if (errno == EINTR)
                            continue;
                        else { perror("accept"); break; }
                    }
                    if (cfd >= FD_MAX) { close(cfd); continue; }
                    set_nonblocking(cfd);
                    conns[cfd].len = conns[cfd].sent = 0;
                    struct epoll_event ev;
                    memset(&ev, 0, sizeof(ev));
                    ev.events  = EPOLLIN;
                    ev.data.fd = cfd;
                    if (epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev) == -1)
                    {
                        perror("epoll_ctl ADD client");
                        close(cfd);
                        continue;
                    }
                }
            }
            else
            {
                uint32_t e = events[i].events;
                int drop = 0;

                // ядро сообщило об ошибке/обрыве на этом fd — снять без разговоров
                if (e & (EPOLLERR | EPOLLHUP))
                    drop = 1;

                // сокет снова готов принимать — дослать накопленный остаток
                if (!drop && (e & EPOLLOUT))
                    if (flush_out(epfd, fd) == -1)
                        drop = 1;

                // данные от клиента
                if (!drop && (e & EPOLLIN))
                {
                    conn_t *c = &conns[fd];
                    // Читаем новую порцию, только если прошлая уже отправлена целиком
                    // (flow control: иначе backlog рос бы без границы). В level-triggered
                    // недочитанное повторно поднимет EPOLLIN на следующем обороте.
                    if (c->len == 0)
                    {
                        ssize_t r = recv(fd, c->buf, sizeof(c->buf), 0);
                        if (r > 0)
                        {
                            c->len  = (size_t)r;
                            c->sent = 0;
                            if (flush_out(epfd, fd) == -1)
                                drop = 1;
                        }
                        else if (r == 0)
                            drop = 1;                        // EOF: клиент закрыл соединение
                        else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
                        {
                            perror("recv");
                            drop = 1;                        // реальная ошибка
                        }
                    }
                }
                if (drop)
                {
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);
                    conns[fd].len = conns[fd].sent = 0;
                }
            }
        }
    }
    close(listen_fd);
    close(epfd);
    exit(EXIT_SUCCESS);
}