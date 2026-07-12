#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>

int main(int argc, char **argv)
{
    if (argc < 2 || argc > 3) // only "2" and "3" are valid
    {
        fprintf(stderr, "usage: %s <hostname> [service]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    struct addrinfo hints, *info;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    const char *host = argv[1];
    const char *service = NULL;
    if (argc == 3)
    {
        service = argv[2];
    }
    else { service = "(default)"; }
    int rv = getaddrinfo(host, service, &hints, &info);
    if (rv != 0) {
        fprintf(stderr, "%s\n", gai_strerror(rv));
        exit(EXIT_FAILURE);
    }


    fprintf(stdout, "%s -> %s\n", host, service);
    for (struct addrinfo *p = info; p != NULL; p = p->ai_next)
    {
        struct sockaddr_in *v4 = NULL;
        struct sockaddr_in6 *v6 = NULL;
        char buf[INET6_ADDRSTRLEN] = {0};
        if (p->ai_family == AF_INET)
        {
            v4 = (struct sockaddr_in *)p->ai_addr;
            if (inet_ntop(v4->sin_family, &(v4->sin_addr), buf, sizeof(buf)) == NULL) 
            {
                perror("inet_ntop");
            }
            uint16_t port = ntohs(v4->sin_port);
            fprintf(stdout, "IPv4 %-50s%u\n", buf, port);
        } 
        else if (p->ai_family == AF_INET6)
        {
            v6 = (struct sockaddr_in6 *)p->ai_addr;
            if (inet_ntop(v6->sin6_family, &(v6->sin6_addr), buf, sizeof(buf)) == NULL) 
            {
                perror("inet_ntop");
            }
            uint16_t port = ntohs(v6->sin6_port);
            fprintf(stdout, "IPv6 %-50s%u\n", buf, port);
        }
    }
    freeaddrinfo(info);

    return EXIT_SUCCESS;
}