#include <sys/epoll.h>     // epoll_create1, epoll_ctl, epoll_wait, struct epoll_event, EPOLLIN
#include <sys/socket.h>    // socket, bind, listen, accept, setsockopt, recv, send, SO_REUSEADDR
#include <netinet/in.h>    // struct sockaddr_in, htons, INADDR_ANY
#include <arpa/inet.h>     // inet_ntop (если печатаешь адрес клиента)
#include <fcntl.h>         // fcntl, F_GETFL, F_SETFL, O_NONBLOCK
#include <unistd.h>        // close, read/write если будешь ими
#include <signal.h>        // sigaction / signal, SIG_IGN, SIGINT, SIGPIPE
#include <errno.h>         // errno, EAGAIN, EWOULDBLOCK, EINTR, EPIPE
#include <string.h>        // memset, strerror
#include <stdio.h>         // perror, fprintf, printf
#include <stdlib.h>        // exit, atoi

#define MAX_EVENTS 64      // размер массива-приёмника для epoll_wait
#define BACKLOG    16      // размер accept queue для listen()
#define BUF_SIZE   4096

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
        return -1;
    }

    if (listen(listening, BACKLOG) == -1)
    {
        perror("listen");
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
    for (;;)
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
                    set_nonblocking(cfd);
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
                struct epoll_event ev;
                memset(&ev, 0, sizeof(ev));
                ev.events  = EPOLLIN;
                ev.data.fd = fd;
                char buf[512]; ssize_t r;
                while ((r = recv(fd, buf, sizeof(buf), 0)) > 0)
                {
                    send(fd, buf, r, 0);
                }
                if (r == 0) {epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev); close(fd); }
                else if (r == -1 && errno != EAGAIN ) { perror("recv"); epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev); close(fd); }
            }
        }
    }

    close(listen_fd);
    close(epfd);
    exit(EXIT_SUCCESS);
}