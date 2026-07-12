#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

static volatile sig_atomic_t stop = 0;
static void on_sigint(int sig) { (void)sig; stop = 1; }

#define BUFSZ 8

static int run_server(const char *port_str);
static int run_client(const char *ip, const char *port_str, const char *msg);

typedef unsigned long long ull;

ull get_ull(const char *inp)
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
    if (value == 0 || value > 65535)
    {
        fprintf(stderr, "error: port must be >0 & <65536\n");
        exit(EXIT_FAILURE);
    }
    return value;
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "usage: %s <server> <port> or %s <ip> <port> <message>\n", argv[0], argv[0]);
        exit(EXIT_FAILURE);
    }

    int res = 0;
    if (strcmp(argv[1], "server") == 0)
        res = run_server(argv[2]);
    else if (strcmp(argv[1], "client") == 0 && argc == 5)
        res = run_client(argv[2], argv[3], argv[4]);
    else {
        fprintf(stderr, "input error\n");
        exit(EXIT_FAILURE);
    }

    return res;
}

static int run_server(const char *port_str)
{
    ull port = get_ull(port_str);

    int server = socket(AF_INET, SOCK_DGRAM, 0);
    if (server == -1) { perror("socket"); exit(EXIT_FAILURE); }

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
    sa.sa_handler = on_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    while (!stop)
    {
        struct sockaddr_in src;
        socklen_t srclen = sizeof(src);
        char buf[BUFSZ] = {0};
        ssize_t n = recvfrom(server, buf, BUFSZ, 0, (struct sockaddr *)&src, &srclen);
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("recvfrom"); 
            break;
        }
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &src.sin_addr, ip, INET_ADDRSTRLEN);
        fprintf(stderr, "client %s:%u\n", ip, ntohs(src.sin_port));
        sendto(server, buf, n, 0, (struct sockaddr *)&src, srclen);
    }
    close(server);
    return 0;
}

static int run_client(const char *ip, const char *port_str, const char *msg)
{
    ull port = get_ull(port_str);

    int client = socket(AF_INET, SOCK_DGRAM, 0);
    if (client == -1) { perror("socket"); exit(EXIT_FAILURE); }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1) { 
        perror("inet_pton"); exit(EXIT_FAILURE);
    }
    addr.sin_port = htons(port);

    size_t len = strlen(msg);
    ssize_t sent = sendto(client, msg, len, 0, (struct sockaddr *)&addr, sizeof(addr));
    if (sent < 0) { perror("sendto"); return 1; }

    struct timeval tv = { .tv_sec = 2, .tv_usec = 0 };
    setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
    
    struct sockaddr_in src;
    socklen_t srclen = sizeof(src);
    char buf[BUFSZ] = {0};
    ssize_t n = recvfrom(client, buf, BUFSZ, 0, (struct sockaddr *)&src, &srclen);
    if (n > 0) { printf("%.*s\n", (int)n, buf); }
    else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        fprintf(stderr, "timeout 2 sec\n"); return 1;
    }
    else if (n < 0) { perror("recvfrom"); return 1; }
    close(client);

    return 0;
}