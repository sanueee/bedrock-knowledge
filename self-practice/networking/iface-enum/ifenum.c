#include <stdio.h>       // printf, fprintf, fopen, fgets, fclose, sscanf
#include <stdlib.h>      // strtoul, exit
#include <string.h>      // strcmp, memset
#include <stdint.h>      // uint32_t
#include <ifaddrs.h>     // getifaddrs, freeifaddrs, struct ifaddrs
#include <sys/socket.h>  // AF_INET, struct sockaddr, sa_family_t
#include <netinet/in.h>  // struct sockaddr_in, struct in_addr
#include <arpa/inet.h>   // inet_ntop, ntohl, htonl

int main()
{
    struct ifaddrs *head;
    int res = getifaddrs(&head);
    if (head == NULL || res == -1)
    {
        perror("getifaddrs"); exit(EXIT_FAILURE);
    }

    for (struct ifaddrs *p = head; p != NULL; p = p->ifa_next)
    {
        if (p->ifa_addr == NULL || p->ifa_addr->sa_family != AF_INET)
            continue;

        struct sockaddr_in *addr = (struct sockaddr_in *)p->ifa_addr;
        char buf_ip[INET_ADDRSTRLEN];
        const char *res = inet_ntop(AF_INET, &addr->sin_addr, buf_ip, sizeof buf_ip);
        if (res == NULL)
        {
            perror("inet_ntop"); continue;
        }

        if (p->ifa_netmask == NULL)
            continue;

        struct sockaddr_in *mask = (struct sockaddr_in *)p->ifa_netmask;
        
        int mask_bit = __builtin_popcount(mask->sin_addr.s_addr);

        FILE *f = fopen("/proc/net/route", "r");
        if (f == NULL)
        {
            fprintf(stderr, "no such file or wrong path: /proc/net/route\n"); break;
        }
        char buf[512];
        fgets(buf, sizeof buf, f);   // пропустить строку-заголовок

        int has_gw = 0;
        char buf_gw[INET_ADDRSTRLEN];
        while (fgets(buf, sizeof buf, f) != NULL)
        {
            char iface[16]; unsigned int dest, gw;
            sscanf(buf, "%15s %x %x", iface, &dest, &gw);

            if (strncmp(iface, p->ifa_name, 16) == 0 && dest == 0)
            {
                struct in_addr g;
                g.s_addr = gw;
                inet_ntop(AF_INET, &g, buf_gw, sizeof(buf_gw));
                has_gw = 1;
            }
        }
        fclose(f);

        if (has_gw)
            fprintf(stdout, "%8.*s %12.*s /%d gw %.*s\n", 16, p->ifa_name, INET_ADDRSTRLEN, buf_ip, mask_bit, INET_ADDRSTRLEN, buf_gw);
        else
            fprintf(stdout, "%8.*s %12.*s /%d (no gateway)\n", 16, p->ifa_name, INET_ADDRSTRLEN, buf_ip, mask_bit);
    }

    freeifaddrs(head);
    return 0;
}
