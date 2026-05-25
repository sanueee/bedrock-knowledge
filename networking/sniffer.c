#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <signal.h>
#include <pcap/pcap.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h> // канальный
#include <netinet/ip.h> // сетевой
#include <netinet/tcp.h> // транспортный

#ifdef __APPLE__
#define TH_PSH TH_PUSH
#endif

static pcap_t *g_handler = NULL;

static const struct { uint8_t mask; const char *name; } flag_table[] = {
    { TH_SYN, "SYN" },
    { TH_ACK, "ACK" },
    { TH_FIN, "FIN" },
    { TH_RST, "RST" },
    { TH_PSH, "PSH" },
    { TH_URG, "URG" },
};

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

static void on_sigint(int sig)
{
    if (g_handler) pcap_breakloop(g_handler);
}

static void packet_cb(u_char *user, const struct pcap_pkthdr *h, const u_char *bytes)
{
    (void)user;

    if (h->caplen < sizeof(struct ether_header)) return;
    const struct ether_header *eth_p = (const struct ether_header *)bytes;

    if (ntohs(eth_p->ether_type) != ETHERTYPE_IP) return;

    // MAC
    const uint8_t *s = eth_p->ether_shost;
    const uint8_t *d = eth_p->ether_dhost;

    // IPv4
    if (h->caplen < sizeof(struct ether_header) + sizeof(struct ip)) return;
    const struct ip *ip_p = (const struct ip *)(bytes+sizeof(struct ether_header));
    const u_char *l4 = (const u_char *)ip_p + ip_p->ip_hl * 4;

    char src_ip[INET_ADDRSTRLEN], dst_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ip_p->ip_src, src_ip, sizeof(src_ip));
    inet_ntop(AF_INET, &ip_p->ip_dst, dst_ip, sizeof(dst_ip));

    // TCP
    if (h->caplen < (l4 - bytes) + sizeof(struct tcphdr)) return;
    const struct tcphdr *tcp_p = (const struct tcphdr *)l4;

    uint16_t sport = ntohs(tcp_p->th_sport);
    uint16_t dport = ntohs(tcp_p->th_dport);

    char buf[64] = {0};
    size_t offset = 0;
    for (size_t i = 0; i < ARRAY_LEN(flag_table); i++)
    {
        if (tcp_p->th_flags & flag_table[i].mask) 
        {
            const char *sep = (offset > 0) ? "|" : "";
            int n = snprintf(buf + offset,
                            sizeof(buf) - offset,
                            "%s%s", sep, flag_table[i].name);
            if (n < 0 || (size_t)n >= sizeof(buf) - offset) break; 
            offset += n;
        }
    }
    if (offset == 0) strcpy(buf, "-");

    int payload_len = ntohs(ip_p->ip_len) - ip_p->ip_hl * 4 - tcp_p->th_off * 4;

    printf("%ld  %02x:%02x:%02x:%02x:%02x:%02x -> %02x:%02x:%02x:%02x:%02x:%02x"
        "  %s:%u -> %s:%u  flags=%s  len=%d\n",
        (long)h->ts.tv_sec,
        s[0],s[1],s[2],s[3],s[4],s[5],
        d[0],d[1],d[2],d[3],d[4],d[5],
        src_ip, sport, dst_ip, dport, buf, payload_len);
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <interface>\n<interface> = eth0, lo...\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *interface = argv[1];
    char errbuf[PCAP_ERRBUF_SIZE];

    g_handler = pcap_open_live(interface, 65535, 1, 1000, errbuf);
    if (!g_handler)
    {
        fprintf(stderr, "pcap_open_live: %s\n", errbuf);
        exit(EXIT_FAILURE);
    }

    struct bpf_program fp;
    if (pcap_compile(g_handler, &fp, "tcp", 1, PCAP_NETMASK_UNKNOWN) < 0)
    {
        fprintf(stderr, "pcap_compile: %s\n", pcap_geterr(g_handler));
        pcap_close(g_handler);
        exit(EXIT_FAILURE);
    }
    if (pcap_setfilter(g_handler, &fp) < 0)
    {
        fprintf(stderr, "pcap_setfilter: %s\n", pcap_geterr(g_handler));
        pcap_freecode(&fp);
        pcap_close(g_handler);
        exit(EXIT_FAILURE);
    }
    pcap_freecode(&fp);   // bytecode уже в ядре, локальную копию можно освободить
    
    struct sigaction sa = { .sa_handler = on_sigint };
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    pcap_loop(g_handler, -1, packet_cb, NULL);
    
    struct pcap_stat st; 
    if (pcap_stats(g_handler, &st) == 0)
        fprintf(stderr, "received=%u dropped=%u\n", st.ps_recv, st.ps_drop);

    pcap_close(g_handler);
    return 0;
}