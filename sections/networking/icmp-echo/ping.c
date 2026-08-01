#include <stdio.h>           // printf, fprintf, perror
#include <stdlib.h>          // exit, EXIT_FAILURE, strtol
#include <string.h>          // memset, memcpy
#include <stdint.h>          // uint8_t, uint16_t, uint32_t
#include <unistd.h>          // close, getpid
#include <errno.h>           // errno, EINTR, EAGAIN, EWOULDBLOCK
#include <time.h>            // clock_gettime, CLOCK_MONOTONIC, struct timespec, nanosleep
#include <sys/socket.h>      // socket, sendto, recvfrom, setsockopt, SOL_SOCKET, SO_RCVTIMEO
#include <sys/time.h>        // struct timeval (аргумент SO_RCVTIMEO)
#include <arpa/inet.h>       // inet_pton, inet_ntop, htons, ntohs
#include <netinet/in.h>      // struct sockaddr_in, IPPROTO_ICMP, INET_ADDRSTRLEN
#include <netinet/ip.h>      // struct iphdr
#include <netinet/ip_icmp.h> // struct icmphdr, ICMP_ECHO, ICMP_ECHOREPLY, ICMP_DEST_UNREACH, ICMP_TIME_EXCEEDED
#include <netdb.h>           // getaddrinfo, freeaddrinfo, gai_strerror
#include <cap-ng.h>          // capng_get_caps_process, capng_clear, capng_apply

// consts
#define PAYLOAD_SIZE 56
#define HEADER_SIZE sizeof(struct icmphdr)
#define ECHO_SIZE (PAYLOAD_SIZE + HEADER_SIZE)
#define DEFAULT_COUNT 4
#define RECV_TIMEOUT_S 1
#define RECV_SIZE 1500

enum outcome
{
    REPLY_OK,
    REPLY_TIMEOUT,
    REPLY_ICMP_ERROR,
    REPLY_FATAL
};

// stats
double rtt_min = 0;
double rtt_avg = 0;
double rtt_max = 0;
int packet_loss = 0;
int transmitted = 0;
int received = 0;

// prototypes
int setup_addr(const char *input, struct sockaddr_in *addr);
int setup_socket(int *fd);
void start_print(struct sockaddr_in *addr);
void ping(int fd, struct sockaddr_in *addr, int counter);
void setup_buf(uint8_t *buf, int id, int seq);
int send_msg(const int fd, const uint8_t *buf, struct sockaddr_in *addr);
int recv_msg(int fd, uint16_t id, int seq, struct timespec t_send, double *rtt, int *ttl, size_t *icmp_len, struct in_addr *src_addr, int *err_type);
int answer_to_structs(const uint8_t *answer, const ssize_t answer_size, struct iphdr **ip, ssize_t *ip_len, struct icmphdr **icmp);
void stats_print();
uint16_t compute_checksum(const uint8_t *data, size_t len);
long long ts_to_ms(struct timespec ts);
long get_long(const char *inp);

int main(int argc, char **argv)
{
    if (argc < 2 || argc > 3)
    {
        fprintf(stderr, "usage: %s <host> [count]\n", argv[0]); exit(EXIT_FAILURE);
    }

    long counter = DEFAULT_COUNT;
    if (argc == 3)
        counter = get_long(argv[2]);

    struct sockaddr_in addr;
    if (setup_addr(argv[1], &addr) == 1)
        exit(EXIT_FAILURE);

    int fd = 0;
    if (setup_socket(&fd) == 1)
        exit(EXIT_FAILURE);
    
    capng_clear(CAPNG_SELECT_CAPS);
    if (capng_apply(CAPNG_SELECT_CAPS) < 0)
    {
        fprintf(stderr, "error: capng_apply\n"); exit(EXIT_FAILURE);
    }

    start_print(&addr);
    ping(fd, &addr, counter);
    stats_print();

    close(fd);
    exit(EXIT_SUCCESS);
}

int setup_addr(const char *input, struct sockaddr_in *addr)
{
    memset(addr, 0, sizeof(struct sockaddr_in));
    addr->sin_family = AF_INET;
    if (inet_pton(AF_INET, input, &addr->sin_addr) <= 0)
    {
        fprintf(stderr, "error: wrong host\n"); return 1;
    }
    return 0;
}

int setup_socket(int *fd)
{
    *fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (*fd == -1)
    {
        if (errno == EPERM)
            fprintf(stderr, "error: operation not permitted (EPERM) \n \
                    try: sudo setcap cap_net_raw+ep ./ping\n");
        else
            perror("socket"); 
        return 1;
    }

    struct timeval tv = { .tv_sec = RECV_TIMEOUT_S, .tv_usec = 0 };
    setsockopt(*fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    return 0;
}

void start_print(struct sockaddr_in *addr)
{
    char addr_buf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr->sin_addr, addr_buf, INET_ADDRSTRLEN);
    printf("PING %s %d bytes of data:\n", addr_buf, PAYLOAD_SIZE);
}

void ping(int fd, struct sockaddr_in *addr, int counter)
{
    char src_buf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr->sin_addr, src_buf, INET_ADDRSTRLEN);
    int seq = 1; uint16_t id = getpid() & 0xFFFF;
    while (seq <= counter)
    {
        uint8_t buf[ECHO_SIZE];
        setup_buf(buf, id, seq);

        struct timespec t_send;
        clock_gettime(CLOCK_MONOTONIC, &t_send);

        if (send_msg(fd, buf, addr) == 1)
        {
            fprintf(stderr, "error: send msg error\n"); seq++; continue;
        }

        double cur_rtt = 0; int ttl = 0; size_t icmp_len = 0; 
        struct in_addr src_addr; int err_type;
        int res = recv_msg(fd, id, seq, t_send, &cur_rtt, &ttl, &icmp_len, &src_addr, &err_type);
        if (res == REPLY_OK)
        {
            received++;
            rtt_avg += cur_rtt;
            if (rtt_min == 0 || rtt_min > cur_rtt)
                rtt_min = cur_rtt;
            if (rtt_max == 0 || rtt_max < cur_rtt)
                rtt_max = cur_rtt;
            char addr_buf[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &src_addr, addr_buf, INET_ADDRSTRLEN);
            printf("%zu bytes from %s: icmp_seq=%d ttl=%d time=%lf ms\n", icmp_len, addr_buf, seq, ttl, cur_rtt);
        }
        else if (res == REPLY_TIMEOUT)
        {
            fprintf(stderr, "timeout %d sec\n", RECV_TIMEOUT_S);
        }
        else if (res == REPLY_ICMP_ERROR)
        {
            if (err_type == ICMP_DEST_UNREACH)
                fprintf(stderr, "destination unreacheble\n");
            else if (err_type == ICMP_TIME_EXCEEDED)
                fprintf(stderr, "time exceeded\n");
        }
        else if (res == REPLY_FATAL)
        {
            fprintf(stderr, "unknown error: recv\n");
        }

        seq++;
        sleep(2);
    }
}

void setup_buf(uint8_t *buf, int id, int seq)
{
    struct icmphdr header;
    memset(&header, 0, HEADER_SIZE);
    header.type = ICMP_ECHO;
    header.code = 0;
    header.checksum = 0;
    header.un.echo.id = htons((uint16_t)id);
    header.un.echo.sequence = htons((uint16_t)seq);
    uint8_t packet[PAYLOAD_SIZE] = {0};
    for (int i = 0; i < PAYLOAD_SIZE; i++)
        packet[i] = i;

    memcpy(buf, &header, HEADER_SIZE);
    memcpy(buf + HEADER_SIZE, packet, PAYLOAD_SIZE);
    header.checksum = htons(compute_checksum(buf, ECHO_SIZE));
    memcpy(buf, &header, HEADER_SIZE);
}

int send_msg(const int fd, const uint8_t *buf, struct sockaddr_in *addr)
{
    ssize_t sent = 0;
    while ((sent = sendto(fd, buf, ECHO_SIZE, 0, (struct sockaddr *)addr, sizeof(*addr))) && errno == EINTR)
        errno = 0;
    if (sent < 0)
    {
        perror("sendto"); return 1;
    }
    transmitted++; 
    return 0;
}

int recv_msg(int fd, uint16_t id, int seq, struct timespec t_send, double *rtt, int *ttl, size_t *icmp_len, struct in_addr *src_addr, int *err_type)
{
    long long deadline = ts_to_ms(t_send) + RECV_TIMEOUT_S * 1000;
    while (1)
    {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        long long remaining = deadline - ts_to_ms(now);
        if (remaining <= 0)
            return REPLY_TIMEOUT;

        struct timeval tv = { .tv_sec = remaining / 1000, .tv_usec = (remaining % 1000) * 1000 };
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        uint8_t answer[RECV_SIZE];
        struct sockaddr_in src; socklen_t srclen = sizeof(src);
        ssize_t recv = recvfrom(fd, answer, RECV_SIZE, 0, (struct sockaddr *)&src, &srclen);
        if (recv < 0)
        {
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return REPLY_TIMEOUT;
            perror("recvfrom"); return REPLY_FATAL;
        }

        struct iphdr *ip = NULL; struct icmphdr *icmp = NULL;
        ssize_t ip_len = 0;
        if (answer_to_structs(answer, recv, &ip, &ip_len, &icmp) == 1)
              continue;

        if (icmp->type == ICMP_ECHOREPLY)
        {
            if (ntohs(icmp->un.echo.id) != id || ntohs(icmp->un.echo.sequence) != seq)
                continue;
            struct timespec t_recv;
            clock_gettime(CLOCK_MONOTONIC, &t_recv);
            *rtt = (t_recv.tv_sec - t_send.tv_sec) * 1000.0 + (t_recv.tv_nsec - t_send.tv_nsec) / 1000000.0;
            *ttl = ip->ttl;
            *icmp_len = recv - ip_len;
            *src_addr = src.sin_addr;
            return REPLY_OK;
        }
        else if (icmp->type == ICMP_DEST_UNREACH || icmp->type == ICMP_TIME_EXCEEDED)
        {
            *err_type = icmp->type;
            *src_addr = src.sin_addr;
            return REPLY_ICMP_ERROR;
        }
    }
}

int answer_to_structs(const uint8_t *answer, const ssize_t answer_size, struct iphdr **ip, ssize_t *ip_len, struct icmphdr **icmp)
{
    if (answer_size >= sizeof(struct iphdr))
    {
        *ip = (struct iphdr *)answer;
        *ip_len = (*ip)->ihl * 4;
    }
    else
        return 1;

    if (*ip_len >= 20 && answer_size >= (*ip_len + sizeof(struct icmphdr)))
        *icmp = (struct icmphdr *)(answer + *ip_len);
    else
        return 1;

    return 0;
}

void stats_print()
{
    printf("-- ping statistics --\n");

    if (transmitted != 0)
        packet_loss = (int)((1 - ((double)received / (double)transmitted)) * 100);
    printf("%d packets transmitted, %d received, %d%% packet loss\n", transmitted, received, packet_loss);

    if (received != 0)
    {
        rtt_avg /= received;
        printf("rtt min/avg/max = %lf/%lf/%lf ms\n", rtt_min, rtt_avg, rtt_max);
    }
}

uint16_t compute_checksum(const uint8_t *data, size_t len)
{
    if (data == NULL) return 0;

    uint32_t sum = 0;
    while (len > 1)
    {   
        uint16_t word = 0;
        word = *data << 8 | *(data + 1);
        sum += (uint32_t)word;
        sum = (sum & 0xFFFF) + (sum >> 16);
        len -= 2;
        data += 2;
    }
    if (len == 1)
    {
        sum += *data << 8;
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    sum = (sum & 0xFFFF) + (sum >> 16);

    return (uint16_t)~sum;
}

long long ts_to_ms(struct timespec ts)
{
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

long get_long(const char *inp)
{
    char *endptr;
    errno = 0;
    long value = strtol(inp, &endptr, 10);
    if (inp == endptr)
    {
        fprintf(stderr, "error: %s. int required.\n", inp);
        exit(EXIT_FAILURE);
    }
    if (errno == ERANGE || value > 1000) // limit for ping
    {
        fprintf(stderr, "error: %s. int overflow\n", inp);
        exit(EXIT_FAILURE);
    }

    return value;
}
