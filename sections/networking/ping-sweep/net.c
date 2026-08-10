#include <stdio.h>           // printf, fprintf, perror, snprintf
#include <stdlib.h>          // exit, EXIT_FAILURE, strtol, calloc, free
#include <string.h>          // memset, memcpy, strchr, strncpy
#include <stdint.h>          // uint8_t, uint16_t, uint32_t
#include <float.h>
#include <unistd.h>          // close, getpid
#include <errno.h>           // errno, EINTR, EAGAIN, EWOULDBLOCK
#include <time.h>            // clock_gettime, CLOCK_MONOTONIC, struct timespec, nanosleep
#include <sys/socket.h>      // socket, sendto, recvfrom, setsockopt, SOL_SOCKET, SO_RCVTIMEO, SO_RCVBUF
#include <sys/time.h>        // struct timeval
#include <arpa/inet.h>       // inet_pton, inet_ntop, htons, ntohs, htonl, ntohl
#include <netinet/in.h>      // struct sockaddr_in, struct in_addr, IPPROTO_ICMP, INET_ADDRSTRLEN
#include <netinet/ip.h>      // struct iphdr
#include <netinet/ip_icmp.h> // struct icmphdr, ICMP_ECHO, ICMP_ECHOREPLY, ICMP_DEST_UNREACH, ICMP_TIME_EXCEEDED
#include <ifaddrs.h>         // getifaddrs, freeifaddrs, struct ifaddrs   (этап 4)
#include <cap-ng.h>          // capng_clear, capng_apply

#include "net.h"

static size_t hosts_up = 0;
static size_t hosts_unreachable = 0;
static size_t probes_sent = 0;
static double rtt_min = DBL_MAX;
static double rtt_sum = 0;
static double rtt_max = 0;

int parse_cidr(char *input, int count, struct in_addr *ip, int *prefix_len)
{
    if (count >= 2)
    {
        char *separator = strchr(input, '/');
        if (separator == NULL)
            return PARSE_NO_SLASH;
        char *endptr;
        errno = 0;
        long v = strtol(separator + 1, &endptr, 10);
        if (endptr == separator + 1 || *endptr != '\0')
            return PARSE_BAD_PREFIX;
        if (v > 32 || v < 0)
            return PARSE_PREFIX_RANGE;

        char addr_buf[INET_ADDRSTRLEN];
        size_t addr_len = (size_t)(separator - input);
        if (addr_len >= sizeof(addr_buf))
            return PARSE_ADDR_TOO_LONG;
        memcpy(addr_buf, input, addr_len);
        addr_buf[addr_len] = '\0';
        if (inet_pton(AF_INET, addr_buf, ip) <= 0)
            return PARSE_BAD_ADDR;

        *prefix_len = v;
    }
    else // by default
    {
        defnet_status ds = get_default_network(ip, prefix_len);
        if (ds != DEFNET_OK)
            return PARSE_NO_DEFAULT_NET;
    }

    if (*prefix_len < 22)
        return PARSE_PREFIX_TOO_WIDE;
    return PARSE_OK;
}

int get_default_network(struct in_addr *ip, int *prefix_len)
{
    FILE *f = fopen("/proc/net/route", "r");
    if (f == NULL)
        return DEFNET_NO_ROUTE_FILE;
    char line[512];                 // одна строка таблицы маршрутов
    char default_iface[16] = {0};   // имя интерфейса с default route — результат первого этапа
    fgets(line, sizeof(line), f);   // пропустить строку-заголовок
    int found_default = 0;
    while (fgets(line, sizeof(line), f) != NULL)
    {
        char iface[16]; unsigned int dest, flags, route_mask;
        if (sscanf(line, "%15s %x %*x %x %*x %*x %*x %x", iface, &dest, &flags, &route_mask) != 4)
            continue;
        if (dest == 0 && route_mask == 0 && (flags & 0x0001))
        {
            strcpy(default_iface, iface);   // %15s гарантировал '\0' и длину <= 15
            found_default = 1;
            break;
        }
    }
    fclose(f);
    if (!found_default)
        return DEFNET_NO_DEFAULT_ROUTE;

    struct ifaddrs *head = NULL;
    int res = getifaddrs(&head);
    if (res == -1)
        return DEFNET_GETIFADDRS_FAILED;
    int found = 0;
    for (struct ifaddrs *p = head; p != NULL; p = p->ifa_next)
    {
        if (p->ifa_addr == NULL || p->ifa_addr->sa_family != AF_INET
            || p->ifa_netmask == NULL || strcmp(p->ifa_name, default_iface) != 0)
            continue;

        struct sockaddr_in *addr = (struct sockaddr_in *)p->ifa_addr;
        struct sockaddr_in *netmask = (struct sockaddr_in *)p->ifa_netmask;
        *ip = addr->sin_addr;
        *prefix_len = __builtin_popcount(netmask->sin_addr.s_addr);
        found = 1;
        break;
    }
    freeifaddrs(head);
    if (!found)
        return DEFNET_NO_IFACE_ADDR;
    return DEFNET_OK;
}

int parse_timeout(char *input, long long *timeout_ms)
{
    char *endptr;
    errno = 0;
    long long value = strtoll(input, &endptr, 10);
    if (endptr == input || *endptr != '\0')
        return PARSE_BAD_TIMEOUT;
    if (errno == ERANGE || value < TIMEOUT_MIN_MS || value > TIMEOUT_MAX_MS)
        return PARSE_BAD_TIMEOUT;
    *timeout_ms = (long long)value;
    return PARSE_OK;
}

int create_target_list(struct target **list, size_t *count, int prefix_len, struct in_addr in)
{
    uint32_t mask = (prefix_len) == 0 ? 0 : (0xFFFFFFFFu << (32 - prefix_len));
    uint32_t network = ntohl(in.s_addr) & mask;
    uint32_t broadcast = network | ~mask;
    *count = (broadcast == network) ? 0 : (broadcast - network - 1);
    if (*count == 0)
    {
        fprintf(stderr, "empty list\n"); return 1;
    }

    *list = calloc(*count, sizeof(struct target));
    if (*list == NULL)
    {
        fprintf(stderr, "allocation target list error\n"); return 1;
    }

    for (size_t i = 0; i < *count; i++)
        (*list)[i].addr.s_addr = htonl(network + 1 + i);

    /*
    for (size_t i = 0; i < *count; i++)
    {
        char buf_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(*list)[i].addr, buf_ip, sizeof(buf_ip));
        printf("%zu:%s\n", i + 1, buf_ip);
    }
    */

    return 0;
}

int send_packets(int *fd, const uint16_t id, const size_t count, struct target *list, long long *t_end_send)
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

    size_t want = count * RCV_BUF_PER_TARGET;
    if (want > RCV_BUF_MAX)
        want = RCV_BUF_MAX;
    int rcvbuf = (int)want;
    if (setsockopt(*fd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf)) != 0)
        perror("setsockopt(SO_RCVBUF)");   // не фатально: работаем с дефолтом

    // ядро возвращает удвоенное значение; меньше запрошенного = упёрлись в rmem_max
    int rcvbuf_actual = 0;
    socklen_t rcvbuf_len = sizeof(rcvbuf_actual);
    if (getsockopt(*fd, SOL_SOCKET, SO_RCVBUF, &rcvbuf_actual, &rcvbuf_len) == 0
        && rcvbuf_actual < rcvbuf)
        fprintf(stderr, "warning: SO_RCVBUF = %d, requested %d (capped by net.core.rmem_max);"
                        " some replies may be dropped\n", rcvbuf_actual, rcvbuf);

    capng_clear(CAPNG_SELECT_CAPS);
    if (capng_apply(CAPNG_SELECT_CAPS) < 0)
    {
        fprintf(stderr, "error: capng_apply\n");
        return 1;
    }

    for (size_t i = 0; i < count; i++)
    {
        struct target *cur_target = &list[i];
        uint8_t buf[ECHO_SIZE];
        setup_buf(buf, id, (int)(i + 1));

        clock_gettime(CLOCK_MONOTONIC, &cur_target->t_send);
        if (send_msg(*fd, buf, &cur_target->addr) != 0)
            fprintf(stderr, "error: send msg error\n");
        else
            probes_sent++;
    }

    *t_end_send = timespec_to_ms(&list[count - 1].t_send);

    return 0;
}

int send_msg(const int fd, const uint8_t *buf, const struct in_addr *addr)
{
    ssize_t sent = 0;
    struct sockaddr_in sending;
    memset(&sending, 0, sizeof(sending));
    sending.sin_family = AF_INET;
    sending.sin_addr = *addr;
    errno = 0;
    while ((sent = sendto(fd, buf, ECHO_SIZE, 0, (struct sockaddr *)&sending, sizeof(sending))) && errno == EINTR)
        errno = 0;
    if (sent < 0)
    {
        perror("sendto"); return 1;
    }

    struct timespec req = { .tv_sec = 0, .tv_nsec = SEND_GAP_NS };
    struct timespec rem;
    while (nanosleep(&req, &rem) == -1 && errno == EINTR)
        req = rem;

    return 0;
}

void setup_buf(uint8_t *buf, const int id, const int seq)
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

void recv_packets(const int fd, const uint16_t id, const size_t count, struct target *list, const long long deadline_ms)
{
    while (1)
    {
        long long remaining = deadline_ms - now_ms();
        if (remaining <= 0)
            break;

        struct timeval t = { .tv_sec = remaining / 1000, .tv_usec = (remaining % 1000) * 1000 };
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(t));

        uint8_t answer[RECV_SIZE];
        struct sockaddr_in src; socklen_t srclen = sizeof(src);
        ssize_t received = recvfrom(fd, answer, RECV_SIZE, 0, (struct sockaddr *)&src, &srclen);
        if (received < 0)
        {
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) // deadline
                break;
            perror("recvfrom"); continue; // ignore packet error, we have other valid packets
        }

        struct iphdr *ip = NULL;
        struct icmphdr *icmp = NULL;
        ssize_t ip_len = 0;
        if (answer_to_structs(answer, received, &ip, &ip_len, &icmp) == 1)
              continue;

        if (icmp->type == ICMP_ECHOREPLY)
        {
            // skip other packets
            if (ntohs(icmp->un.echo.id) != id)
                continue;
            uint16_t seq = ntohs(icmp->un.echo.sequence);
            if (seq < 1 || seq > count)
                continue;
            if (src.sin_addr.s_addr != list[seq-1].addr.s_addr)
                continue;
            if (list[seq-1].status == HOST_ALIVE)
                continue;

            struct target *cur_target = &list[seq - 1];
            struct timespec t_recv;
            clock_gettime(CLOCK_MONOTONIC, &t_recv);
            cur_target->rtt_ms = (t_recv.tv_sec - cur_target->t_send.tv_sec) * 1000.0 + \
                                    (t_recv.tv_nsec - cur_target->t_send.tv_nsec) / 1000000.0;
            cur_target->ttl = ip->ttl;
            cur_target->status = HOST_ALIVE;

            // stats
            hosts_up++;
            rtt_sum += cur_target->rtt_ms;
            if (cur_target->rtt_ms < rtt_min)
                rtt_min = cur_target->rtt_ms;
            if (cur_target->rtt_ms > rtt_max)
                rtt_max = cur_target->rtt_ms;

            // printing
            char src_buf[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &src.sin_addr, src_buf, INET_ADDRSTRLEN);
            printf("%-15s ttl=%d time=%.3lf ms\n", src_buf, cur_target->ttl, cur_target->rtt_ms);
        }
        else if (icmp->type == ICMP_DEST_UNREACH || icmp->type == ICMP_TIME_EXCEEDED)
        {
            ssize_t offset = ip_len + (ssize_t)sizeof(struct icmphdr);

            if (received - offset < (ssize_t)sizeof(struct iphdr))
                continue;

            const struct iphdr *inner_ip = (const struct iphdr *)(answer + offset);
            ssize_t inner_ip_len = (ssize_t)inner_ip->ihl * 4;

            if (inner_ip_len < (ssize_t)sizeof(struct iphdr))
                continue;

            if (received - offset - inner_ip_len < (ssize_t)sizeof(struct icmphdr))
                continue;

            const struct icmphdr *inner_icmp = (const struct icmphdr *)(answer + offset + inner_ip_len);

            if (inner_icmp->type != ICMP_ECHO || ntohs(inner_icmp->un.echo.id) != id)
                continue;

            uint16_t seq = ntohs(inner_icmp->un.echo.sequence);
            if (seq < 1 || seq > count)
                continue;
            if (inner_ip->daddr != list[seq - 1].addr.s_addr)
                continue;
            if (list[seq - 1].status == HOST_NO_REPLY)
            {
                list[seq - 1].status = HOST_UNREACHABLE;
                hosts_unreachable++;
            }
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

void print_summary(const long long elapsed)
{
    double min = 0;
    double avg = 0;
    double max = 0;
    if (hosts_up > 0)
    {
        min = rtt_min;
        avg = rtt_sum / (double)hosts_up;
        max = rtt_max;
    }

    printf("--- sweep statistics ---\n");
    printf("%zu probes sent, %zu hosts up, %zu hosts unreachable, %zu no response\n", probes_sent, hosts_up, \
            hosts_unreachable, probes_sent - hosts_up - hosts_unreachable);
    printf("elapsed %lld ms, rtt min/avg/max = %.3lf/%.3lf/%.3lf ms\n", elapsed, min, avg, max);
}

long long now_ms(void)
{
      struct timespec ts;
      clock_gettime(CLOCK_MONOTONIC, &ts);
      return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

long long timespec_to_ms(const struct timespec *t)
{
    return (t->tv_sec * 1000 + t->tv_nsec / 1000000);
}

int get_int(const char *inp)
{
    char *endptr;
    errno = 0;
    long value = strtol(inp, &endptr, 10);
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

    return (int)value;
}
