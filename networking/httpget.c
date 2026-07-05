#define _GNU_SOURCE

#include <stddef.h>
#include <stdio.h>      // printf, fprintf, snprintf
#include <stdlib.h>     // strtol (код статуса, hex-размер чанка), exit
#include <string.h>     // strstr, strncmp, memcpy, strlen
#include <strings.h>    // strcasecmp, strncasecmp (case-insensitive имена заголовков)
#include <errno.h>      // errno
#include <unistd.h>     // close
#include <netdb.h>      // getaddrinfo, freeaddrinfo, gai_strerror, struct addrinfo
#include <sys/socket.h> // socket, connect, recv, send, struct sockaddr, AF_*, SOCK_STREAM
#include <arpa/inet.h>

#define REQ_SIZE 256
#define ANS_SIZE 8192

typedef enum { FRAMING_CHUNKED, FRAMING_LENGTH, FRAMING_CLOSE } func_parse_res_t;

int tcp_connect(const char *host, const char *port);
func_parse_res_t parse_headers(const char *buf, const char *sep, long *content_length);
int decode_chunked(const char *body, size_t body_len);

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        fprintf(stderr, "usage: %s <host> <path> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *host = argv[1];
    const char *path = argv[2];
    const char *service = argv[3];

    int fd = tcp_connect(host, service);
    if (fd == -1)
    {
        fprintf(stderr, "error: tcp connection isn`t created\n");
        exit(EXIT_FAILURE);
    }

    char req[REQ_SIZE] = {0};
    ssize_t n = snprintf(req, sizeof req,
      "GET %s HTTP/1.1\r\n"
      "Host: %s\r\n"
      "Connection: close\r\n"
      "\r\n",
      path, host);

    ssize_t sent = 0;
    while (sent < n)
    {
        ssize_t k = send(fd, req + sent, n - sent, 0);
        if (k == -1) { perror("send"); exit(EXIT_FAILURE); }
        sent += k;
    }
    
    size_t total_ans = 0; ssize_t ans = 0;
    char buf[ANS_SIZE];
    while ((ans = recv(fd, buf + total_ans, sizeof buf - total_ans, 0)) > 0)
        total_ans += ans;
    if (ans == -1)
    {
        perror("recv");
        close(fd);
        exit(EXIT_FAILURE);
    }
    const char *sep = memmem(buf, total_ans, "\r\n\r\n", 4);
    if (sep == NULL)
    {
        close(fd);
        fprintf(stderr, "malformed response: no header/body separator\n");
        exit(EXIT_FAILURE);
    }
    const char *body = sep + 4;

    long content_length = 0;
    func_parse_res_t res = parse_headers(buf, sep, &content_length);
    if (res == FRAMING_LENGTH) {
        int len = content_length <= total_ans - (body - buf) ? content_length: total_ans - (body - buf);
        printf("content body:\n");
        printf("%.*s\n", len, body);
    } else if (res == FRAMING_CHUNKED) {
        int res = decode_chunked(body, total_ans - (body - buf));
        if (res == 1) 
            fprintf(stderr, "malformed chunked body\n");
    } else if (res == FRAMING_CLOSE) {
        int len = total_ans - (body - buf);
        printf("content body:\n");
        printf("%.*s\n", len, body);
    }

    close(fd);    
    exit(EXIT_SUCCESS);
}

int tcp_connect(const char *host, const char *port)
{
    struct addrinfo hints, *info;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int rv = getaddrinfo(host, port, &hints, &info);
    if (rv != 0)
    {
        fprintf(stderr, "%s\n", gai_strerror(rv));
        exit(EXIT_FAILURE);
    }

    for (struct addrinfo *p = info; p != NULL; p = p->ai_next)
    {
        int fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd == -1)
        {
            perror("socket");
            continue;
        }

        int res = connect(fd, p->ai_addr, p->ai_addrlen);
        if (res == 0)
        {
            freeaddrinfo(info);
            return fd;
        }
        else 
        {
            close(fd);
            continue;
        }
    }
    freeaddrinfo(info);
    return -1;
}

func_parse_res_t parse_headers(const char *buf, const char *sep, long *content_length)
{
    char *line = (char *)buf;

    long clen = -1; int is_chunked = 0;
    while (line < sep) 
    {
        char *eol = memmem(line, sep - line, "\r\n", 2);
        if (eol == NULL)
            eol = (char *)sep;
        size_t len = eol - line;

        if (line == buf) {
            char *sp = memchr(line, ' ', len);
            if (sp == NULL)
            {
                fprintf(stderr, "malformed header\n");
                exit(EXIT_FAILURE);
            }
            printf("Status: %.*s\n", (int)(len - (sp + 1 - line)), sp + 1);
        } else if (strncasecmp(line, "server:", 7) == 0) {
            char *v = line + 7;
            while (*v == ' ') v++;
            printf("Server: %.*s\n", (int)(eol - v) , v);
        } else if (strncasecmp(line, "Content-Length:", 15) == 0) {
            clen = strtol(line + 15, NULL, 10);
            printf("Content-Length: %ld\n", clen);
        } else if (strncasecmp(line, "Transfer-Encoding:", 18) == 0) {
            if (memmem(line, len, "chunked", 7))
                is_chunked = 1;
        }
        line = eol + 2;
    }

    if (is_chunked) return FRAMING_CHUNKED;
    else if (clen >= 0) { *content_length = clen; return FRAMING_LENGTH; }
    else return FRAMING_CLOSE;
}

int decode_chunked(const char *body, size_t body_len)
{
    printf("chunked content body:\n");
    char *p = (char *)body;
    const char *end = body + body_len;
    char *endptr = p; int fail = 0;
    while (1)
    {
        long size = strtol(p, &endptr, 16);
        if (endptr == p) { fail = 1; break; }
        if (size == 0) break;

        p = endptr + 2;
        if (size > end - p) { fail = 1; break; }
        printf("%.*s", (int)size, p);
        p += size + 2;
    }
    printf("\n");
    return fail;    
}