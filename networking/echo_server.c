#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

volatile sig_atomic_t stop = 0;

void handler(int sig)
{
    if (sig == SIGINT)
    {
        stop = 1;
        write(STDOUT_FILENO, "exit\n", 5);
    }
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s n (n - port)\n", argv[0]);
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

    int val = 1;
    if (setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) == -1)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(server, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    struct sigaction sa;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    struct sigaction sa_pipe = { .sa_handler = SIG_IGN };
    sigaction(SIGPIPE, &sa_pipe, NULL);

    if (listen(server, 8) == -1)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }    
    while (!stop)
    {
        int client_fd;
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        if ((client_fd = accept(server, (struct sockaddr *)&client_addr, &client_addr_len)) == -1)
        {
            if (errno != EINTR)
            {
                perror("accept");
                continue;
            }
            continue;
        }

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip, INET_ADDRSTRLEN);
        fprintf(stderr, "client %s:%u\n", ip, ntohs(client_addr.sin_port));
        while (!stop)
        {
            char buf[4096];
            ssize_t n = recv(client_fd, buf, sizeof(buf), 0);
            if (n == 0)
                break;
            if (n == -1)
            {
                if (errno == EINTR)
                    continue;
                perror("recv");
                break;
            }
            if (n > 0)
            {
                ssize_t s = send(client_fd, buf, n, 0);
                if (s == -1)
                {
                    if (errno == EPIPE || errno == ECONNRESET)
                        break;

                    perror("send");
                    break;
                }
            }
        }
        close(client_fd);
    }
    close(server);
    fprintf(stderr, "bye..\n");
    return 0;
}