#include <sys/epoll.h>     // epoll_create1, epoll_ctl, epoll_wait, struct epoll_event, EPOLLIN
#include <sys/socket.h>    // socket, bind, listen, accept, setsockopt, recv, send, SO_REUSEADDR
#include <netinet/in.h>    // struct sockaddr_in, htons, INADDR_ANY
#include <arpa/inet.h>     // inet_ntop (если печатаешь адрес клиента)
#include <fcntl.h>         // fcntl, F_GETFL, F_SETFL, O_NONBLOCK
#include <unistd.h>        // close, read/write если будешь ими
#include <signal.h>        // sigaction / signal, SIG_IGN, SIGINT, SIGPIPE
#include <errno.h>         // errno, EAGAIN, EWOULDBLOCK, EINTR, EPIPE
#include <string.h>        // memset, strerror
#include <stdio.h>         // perror, fprintf, printf
#include <stdlib.h>        // exit, atoi

int main(int argc, char **argv)
{
    long service = 8080;
    if (argc == 2)
    {
        char *end; errno = 0;
        service = strtol(argv[1], &end, 10);
        if (errno != 0 || end == argv[1] || *end != '\0' || service > 65535 || service < 1)
        {
            fprintf(stderr, "error: argument must be int: 1...65535\n");
            exit(EXIT_FAILURE);
        }
    }

    int epfd = epoll_create1(0);
    epoll_ctl(epfd, EPOLL_CTL_ADD | EPOLL_CTL_MOD | EPOLL_CTL_DEL, )
    
    

    exit(EXIT_SUCCESS);
}