#include <stdio.h>       // printf, fprintf, perror
#include <stdlib.h>      // exit, EXIT_FAILURE
#include <unistd.h>      // close, getuid, geteuid
#include <errno.h>       // errno, EPERM
#include <string.h>      // strerror
#include <sys/socket.h>  // socket, AF_INET, SOCK_RAW
#include <netinet/in.h>  // IPPROTO_ICMP
#include <cap-ng.h>      // capng_* (libcap-ng)

int main(void)
{
    uid_t id = getuid();
    uid_t eid = geteuid();
    if (capng_get_caps_process() == -1)
    {
        fprintf(stderr, "error: get caps\n"); exit(EXIT_FAILURE);
    }

    int res = capng_have_capability(CAPNG_EFFECTIVE, CAP_NET_RAW);
    if (res == 0)
        printf("[caps] CAP_NET_RAW effective? NO\n");

    int fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (fd == -1)
    {
        if (errno == EPERM)
            fprintf(stderr, "[sock] socket(AF_INET, SOCK_RAW, IPPROTO_ICMP): Operation not permitted (EPERM) \n \
                    подсказка: sudo setcap cap_net_raw+ep ./rawcap   (или запуск под root)\n");
        else
            perror("socket"); 
        exit(EXIT_FAILURE);
    }

    printf("[caps] uid=%d euid=%d\n", id, eid);
    if (res == 1) printf("[caps] CAP_NET_RAW effective? YES\n");
    printf("[sock] raw socket opened, fd=%d\n", fd);

    capng_clear(CAPNG_SELECT_CAPS);
    if (capng_apply(CAPNG_SELECT_CAPS) < 0)
    {
        fprintf(stderr, "error: capng_apply\n"); exit(EXIT_FAILURE);
    }

    printf("[drop] cleared all capabilities\n");
    if (capng_get_caps_process() == -1)
    {
        fprintf(stderr, "error: get caps\n"); exit(EXIT_FAILURE);
    }

    if (capng_have_capability(CAPNG_EFFECTIVE, CAP_NET_RAW) == 0)
        printf("[caps] CAP_NET_RAW effective? NO\n");

    int new_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (new_fd == -1)
    {
        if (errno == EPERM)
            fprintf(stderr, "[sock] socket(AF_INET, SOCK_RAW, IPPROTO_ICMP): Operation not permitted (EPERM)\n");
        else
        {
            perror("socket"); 
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        close(new_fd);
        exit(EXIT_FAILURE);
    }
    close(fd);
    return 0;
}
