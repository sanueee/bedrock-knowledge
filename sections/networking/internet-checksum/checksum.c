#include <stdio.h>       // printf, fprintf
#include <stdlib.h>      // exit, EXIT_FAILURE
#include <string.h>      // memset, memcpy
#include <stdint.h>      // uint8_t, uint16_t, uint32_t
#include <arpa/inet.h>   // inet_pton, htons, htonl, struct in_addr
#include <netinet/in.h>  // IPPROTO_TCP, in_addr
#include <netinet/ip.h>  // struct iphdr
#include <netinet/tcp.h> // struct tcphdr
#include <sys/socket.h>

struct pseudo_header {
    uint32_t saddr;
    uint32_t daddr; 
    uint8_t  zero;  
    uint8_t  protocol;
    uint16_t tcp_length;
}; 

uint16_t compute_checksum(const uint8_t *data, size_t len);
int build_ip_header(struct iphdr *header, const char *src_ip, const char *dst_ip);
int build_tcp_header(struct tcphdr *header, const char *src_ip, const char *dst_ip);
int build_pseudo_header(struct pseudo_header *pseudo, const char *src_ip, const char *dst_ip);
int valid_addr(const char *ip, uint32_t *out);
int verify_checksum(const uint8_t *data, size_t len);
void print_bytes(const uint8_t *data, size_t len);

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        fprintf(stderr, "usage: %s <src-ip> <dst-ip>\n", argv[0]); exit(EXIT_FAILURE);
    }

    struct iphdr header_ip;
    struct tcphdr header_tcp;

    if (build_ip_header(&header_ip, argv[1], argv[2]) == -1)
    {
        fprintf(stderr, "error: building ip header\n"); exit(EXIT_FAILURE);
    }
    if (build_tcp_header(&header_tcp, argv[1], argv[2]) == -1)
    {
        fprintf(stderr, "error: building tcp header\n"); exit(EXIT_FAILURE);
    }

    print_bytes((uint8_t *)&header_ip, 20);
    printf("IP checksum valid: %d\n", verify_checksum((uint8_t *)&header_ip, 20));

    exit(EXIT_SUCCESS);
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

int build_ip_header(struct iphdr *header, const char *src_ip, const char *dst_ip)
{
    memset(header, 0, sizeof(struct iphdr));

    uint32_t addr;
    int res = valid_addr(src_ip, &addr);
    if (res == -1)
        return -1;
    else
        header->saddr = addr;

    res = valid_addr(dst_ip, &addr);
    if (res == -1)
        return -1;
    else
        header->daddr = addr;

    header->version = 4;
    header->ihl = 5;
    header->tot_len = htons(20+20);
    header->ttl = 64;
    header->protocol = IPPROTO_TCP;
    header->check = 0;

    header->check = htons(compute_checksum((uint8_t *)header, 20));

    return 0;
}

int build_tcp_header(struct tcphdr *header, const char *src_ip, const char *dst_ip)
{
    memset(header, 0, sizeof(struct tcphdr));

    header->source = htons(54321);
    header->dest = htons(80);
    header->seq = htonl(0);
    header->doff = 5;
    header->syn = 1;
    header->window = htons(65535);
    header->check = 0;

    struct pseudo_header pseudo;
    if (build_pseudo_header(&pseudo, src_ip, dst_ip) == -1)
        return -1;

    uint8_t buffer[sizeof(pseudo) + sizeof(*header)] = {0};
    memcpy(buffer, &pseudo, sizeof(pseudo));
    memcpy(buffer + sizeof(pseudo), header, sizeof(*header));

    header->check = htons(compute_checksum(buffer, sizeof(buffer)));

    return 0;
}

int build_pseudo_header(struct pseudo_header *pseudo, const char *src_ip, const char *dst_ip)
{
    memset(pseudo, 0, sizeof(struct pseudo_header));

    uint32_t addr;
    int res = valid_addr(src_ip, &addr);
    if (res == -1)
        return -1;
    else
        pseudo->saddr = addr;

    res = valid_addr(dst_ip, &addr);
    if (res == -1)
        return -1;
    else
        pseudo->daddr = addr;

    pseudo->protocol = IPPROTO_TCP;
    pseudo->zero = 0;
    pseudo->tcp_length = htons(20);
    return 0;
}

int valid_addr(const char *ip, uint32_t *out)
{
    struct in_addr addr;
    if (inet_pton(AF_INET, ip, &addr) <= 0)
        return -1;
    *out = addr.s_addr;
    return 0;
}

int verify_checksum(const uint8_t *data, size_t len)
{
    return compute_checksum(data, len) == 0;
}

void print_bytes(const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        printf("%02x ", data[i]);
        if ((i + 1) % 16 == 0)
            printf("\n");
    }
    if (len % 16 != 0)
        printf("\n");
}