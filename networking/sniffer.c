#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <signal.h>
#include <pcap/pcap.h>
#include <netinet/if_ether.h> // канальный
#include <netinet/ip.h> // сетевой
#include <netinet/tcp.h> // транспортный

static pcap_t *g_handler = NULL;

static void on_sigint(int sig)
{
    if (sig) pcap_breakloop(g_handler);
}

static void packet_cb(u_char *user, const struct pcap_pkthdr *h, const u_char *bytes)
{
    if (h->caplen < sizeof(struct ether_header)) return;
    const struct ether_header *eth = (const struct ether_header *)bytes;

    if (ntohs(eth->ether_type) == ETHERTYPE_IP) return;

    const uint8_t *s = eth->ether_shost;
    const uint8_t *d = eth->ether_dhost;
    printf("%02x:%02x:%02x:%02x:%02x:%02x -> %02x:%02x:%02x:%02x:%02x:%02x\n",
           s[0],s[1],s[2],s[3],s[4],s[5],
           d[0],d[1],d[2],d[3],d[4],d[5]);
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <intergace>\n<interface> = eth0, lo...\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *interface = argv[1];
    int mode = 0;
    if (strcmp(interface, "eth0") == 0)
    {
        mode = 0;
    }
    else if (strcmp(interface, "lo") == 0)
    {
        mode = 1;
    }

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
        exit(EXIT_FAILURE);
    }
    if (pcap_setfilter(g_handler, &fp) < 0)
    {
        fprintf(stderr, "pcpap_setfilter: %s\n", pcap_geterr(g_handler));
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