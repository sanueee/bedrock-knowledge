#include <stdio.h>           // printf, fprintf, perror, snprintf
#include <stdlib.h>          // exit, EXIT_FAILURE, free
#include <string.h>          // memset, memcpy
#include <stdint.h>          // uint8_t, uint16_t, uint32_t
#include <unistd.h>          // close, getpid
#include <errno.h>           // errno, EINTR, EAGAIN, EWOULDBLOCK
#include <time.h>            // clock_gettime, CLOCK_MONOTONIC, struct timespec, nanosleep
#include <sys/socket.h>      // socket, sendto, recvfrom, setsockopt, SOL_SOCKET, SO_RCVTIMEO, SO_RCVBUF
#include <sys/time.h>        // struct timeval
#include <arpa/inet.h>       // inet_ntop, htons, ntohs, htonl, ntohl
#include <netinet/in.h>      // struct sockaddr_in, struct in_addr, IPPROTO_ICMP, INET_ADDRSTRLEN
#include <netinet/ip.h>      // struct iphdr
#include <netinet/ip_icmp.h> // struct icmphdr, ICMP_ECHO, ICMP_ECHOREPLY, ICMP_DEST_UNREACH, ICMP_TIME_EXCEEDED
#include <cap-ng.h>          // capng_clear, capng_apply
#include "net.h"

int main(int argc, char **argv)
{
    if (argc > 3)
    {
        fprintf(stderr, "usage: %s [CIDR] [timeout_ms]\n", argv[0]); exit(EXIT_FAILURE);
    }

    struct in_addr in;
    int prefix_len = 0;
    long long timeout_ms = TIMEOUT_DEFAULT_MS;
    int error = parse_cidr(argv[1], argc, &in, &prefix_len);
    switch (error)
    {
        case PARSE_OK:
            break;
        case PARSE_NO_SLASH:
            fprintf(stderr, "error: no '/' in argument, expected CIDR (e.g. 172.17.0.0/24)\n");
            exit(EXIT_FAILURE);
        case PARSE_ADDR_TOO_LONG:
            fprintf(stderr, "error: address part is too long\n");
            exit(EXIT_FAILURE);
        case PARSE_BAD_ADDR:
            fprintf(stderr, "error: not a valid IPv4 address\n");
            exit(EXIT_FAILURE);
        case PARSE_BAD_PREFIX:
            fprintf(stderr, "error: prefix is not a number\n");
            exit(EXIT_FAILURE);
        case PARSE_PREFIX_RANGE:
            fprintf(stderr, "error: prefix out of range [0, 32]\n");
            exit(EXIT_FAILURE);
        case PARSE_PREFIX_TOO_WIDE:
            fprintf(stderr, "error: prefix is too wide, too many targets to sweep\n");
            exit(EXIT_FAILURE);
        case PARSE_NO_DEFAULT_NET:
            fprintf(stderr, "error: cannot determine default network, pass CIDR explicitly\n");
            exit(EXIT_FAILURE);
    }

    if (argc == 3 && parse_timeout(argv[2], &timeout_ms) != PARSE_OK)
    {
        fprintf(stderr, "error: bad timeout, expected %d..%d ms\n", TIMEOUT_MIN_MS, TIMEOUT_MAX_MS);
        exit(EXIT_FAILURE);
    }

    struct target *list = NULL;
    size_t count = 0;
    if (create_target_list(&list, &count, prefix_len, in) != 0)
        exit(EXIT_FAILURE);

    long long start = now_ms();

    uint16_t id = getpid() & 0xFFFF;
    int fd = -1; long long t_end_send;
    if (send_packets(&fd, id, count, list, &t_end_send) != 0)
    {
        close(fd);
        free(list);
        exit(EXIT_FAILURE);
    }
    long long deadline_ms = t_end_send + timeout_ms;
    recv_packets(fd, id, count, list, deadline_ms);

    long long end = now_ms();
    long long elapsed = end - start;
    print_summary(elapsed);

    close(fd);
    free(list);
    exit(EXIT_SUCCESS);
}
