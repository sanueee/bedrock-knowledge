#include <stdio.h>       // printf, fprintf, perror
#include <stdlib.h>      // exit, EXIT_FAILURE
#include <string.h>      // memcpy, memchr, strlen, memset
#include <stdint.h>      // uint8_t, uint16_t, uint32_t
#include <unistd.h>      // close
#include <sys/time.h>    // struct timeval
#include <arpa/inet.h>   // htons, ntohs, inet_pton, inet_ntop, INET_ADDRSTRLEN
#include <sys/socket.h>  // socket, send, recv, connect, setsockopt
#include <netinet/in.h>  // struct sockaddr_in
#include <errno.h>       // errno, EAGAIN, EWOULDBLOCK

int  dns_socket(const char *server_ip);

size_t build_query(uint8_t *query, uint16_t id, const char *host);
void encode(const char *name, uint8_t **dst);
int  send_query(int fd, const uint8_t *query, size_t len);

int  parse_flags(const uint8_t *msg);
uint8_t *skip_name(uint8_t *p, const uint8_t *end);
void parse_answers(const uint8_t *msg, uint8_t *p, const uint8_t *end, uint16_t ancount);

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "usage: %s <hostname>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int fd = dns_socket("8.8.8.8");

    uint8_t  query[512];
    uint16_t id = 1234;
    size_t   query_len = build_query(query, id, argv[1]);

    if (send_query(fd, query, query_len) == -1)
        exit(EXIT_FAILURE);

    uint8_t answer[512];
    ssize_t n = recv(fd, answer, sizeof(answer), 0);
    if (n < 0 && (errno == EWOULDBLOCK || errno == EAGAIN))
    {
        fprintf(stderr, "error: timeout\n"); exit(EXIT_FAILURE);
    }
    else if (n < 0) { perror("recv"); exit(EXIT_FAILURE); }

    if (n < 12)
    {
        fprintf(stderr, "malformed: answer shorter than DNS header\n");
        exit(EXIT_FAILURE);
    }
    const uint8_t *end = answer + n;

    uint16_t ans_id = answer[0] << 8 | answer[1];
    if (ans_id != id)
    {
        fprintf(stderr, "id mismatch\n"); exit(EXIT_FAILURE);
    }

    int rcode = parse_flags(answer);
    switch (rcode)
    {
        case -1: fprintf(stderr, "not a response (QR=0)\n"); exit(EXIT_FAILURE);
        case 0:  break;
        case 3:  fprintf(stderr, "NXDOMAIN\n");     exit(EXIT_FAILURE);
        case 5:  fprintf(stderr, "REFUSED\n");      exit(EXIT_FAILURE);
        default: fprintf(stderr, "rcode=%d\n", rcode); exit(EXIT_FAILURE);
    }

    uint16_t ancount = answer[6] << 8 | answer[7];
    if (ancount == 0)
    {
        fprintf(stderr, "NODATA: имя есть, но A-записей нет\n");
        exit(EXIT_FAILURE);
    }
    printf("got %u answer(s):\n", ancount);

    uint8_t *p = skip_name(answer + 12, end);
    if (p == NULL || p + 4 > end)
    {
        fprintf(stderr, "malformed question section\n"); exit(EXIT_FAILURE);
    }
    p += 4;

    parse_answers(answer, p, end, ancount);

    close(fd);
    return 0;
}

int dns_socket(const char *server_ip)
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) { perror("socket"); exit(EXIT_FAILURE); }

    struct timeval tv = { .tv_sec = 2, .tv_usec = 0 };
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(53);
    if (inet_pton(AF_INET, server_ip, &addr.sin_addr) != 1)
    {
        perror("inet_pton"); exit(EXIT_FAILURE);
    }
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("connect"); exit(EXIT_FAILURE);
    }
    return fd;
}

size_t build_query(uint8_t *query, uint16_t id, const char *host)
{
    uint8_t *p = query;

    uint8_t dns_header[12] = {0};
    dns_header[0] = id >> 8;
    dns_header[1] = id & 0xFF;
    dns_header[2] = (uint8_t)1; // RD
    dns_header[5] = (uint8_t)1; // QDCOUNT = 1
    memcpy(p, dns_header, 12); p += 12;

    encode(host, &p);

    *p++ = 0; *p++ = 1; // QTYPE  = A
    *p++ = 0; *p++ = 1; // QCLASS = IN

    return p - query;
}

void encode(const char *name, uint8_t **dst)
{
    const char *end = name + strlen(name);
    const char *p = name;
    while (p < end)
    {
        const char *dot = memchr(p, '.', end - p);
        size_t label = dot ? (size_t)(dot - p) : (size_t)(end - p);
        if (label == 0 || label > 63)
        {
            fprintf(stderr, "malformed label\n"); exit(EXIT_FAILURE);
        }
        *(*dst)++ = (uint8_t)label;
        memcpy(*dst, p, label);
        *dst += label;
        p += label;
        if (dot) p++;
    }
    *(*dst)++ = 0;
}

int send_query(int fd, const uint8_t *query, size_t len)
{
    if (send(fd, query, len, 0) == -1)
    {
        perror("send");
        return -1;
    }
    return 0;
}

int parse_flags(const uint8_t *msg)
{
    uint16_t flags = msg[2] << 8 | msg[3];
    if (((flags >> 15) & 1) != 1)   // QR
        return -1;
    return flags & 0x0F;            // RCODE
}

uint8_t *skip_name(uint8_t *p, const uint8_t *end)
{
    while (1)
    {
        if (p >= end) return NULL;
        uint8_t b = *p;

        if ((b & 0xC0) == 0xC0)         
        {
            if (p + 2 > end) return NULL;
            return p + 2;
        }
        if (b == 0) return p + 1;
        if (b > 63) return NULL;    

        p += 1 + b;     
    }
}

void parse_answers(const uint8_t *msg, uint8_t *p, const uint8_t *end, uint16_t ancount)
{
    for (uint16_t i = 0; i < ancount; i++)
    {
        p = skip_name(p, end);
        if (p == NULL) { fprintf(stderr, "malformed name in RR\n"); return; }

        // TYPE(2) CLASS(2) TTL(4) RDLENGTH(2) = 10 байт фиксированной части
        if (p + 10 > end) { fprintf(stderr, "truncated RR\n"); return; }
        uint16_t type     = p[0] << 8 | p[1];
        uint16_t rdlength = p[8] << 8 | p[9];
        p += 10;

        if (p + rdlength > end) { fprintf(stderr, "bad rdlength\n"); return; }

        if (type == 1 && rdlength == 4) // A-запись
        {
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, p, ip, sizeof(ip));
            printf("  %s\n", ip);
        }
        p += rdlength;
    }
}
