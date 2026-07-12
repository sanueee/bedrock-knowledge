#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, argv[1], &addr.sin_addr) != 1)
    {
        fprintf(stderr, "bad IP: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }
    char *end; errno = 0;
    long port = strtol(argv[2], &end, 10);
    if (errno != 0 || end == argv[2] || *end != '\0' || port > 65535 || port < 1)
    {
        fprintf(stderr, "error: argument must be port 1 ... 65535\n");
        exit(EXIT_FAILURE);
    }
    addr.sin_port = htons(port);
    if (connect(client_fd, (struct sockaddr *)&addr, sizeof addr) == -1)
    {
        perror("connect");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    char buf[4096];
    while (fgets(buf, sizeof(buf), stdin) != NULL)
    {
        size_t n = strlen(buf);
        ssize_t res = send(client_fd, buf, n, 0);
        if (res == -1)
        {
            if (errno == EPIPE || errno == ECONNRESET)
                break;
            perror("send");
            break;
        }
        char ans[4096];
        ssize_t r = recv(client_fd, ans, sizeof(ans), 0);
        if (r == 0)
            break;
        if (r == -1)
        {
            perror("recv");
            break;
        }
        fwrite(ans, 1, r, stdout);
    }
    close(client_fd);
    return 0;
}