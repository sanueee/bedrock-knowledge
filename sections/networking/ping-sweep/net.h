#ifndef NET_H
#define NET_H

#include <stddef.h>          // size_t
#include <stdint.h>          // uint8_t
#include <sys/types.h>       // ssize_t
#include <time.h>            // struct timespec
#include <netinet/in.h>      // struct in_addr
#include <netinet/ip.h>      // struct iphdr
#include <netinet/ip_icmp.h> // struct icmphdr

// consts
#define TIMEOUT_DEFAULT_MS 2000
#define TIMEOUT_MIN_MS     1
#define TIMEOUT_MAX_MS     60000
#define PAYLOAD_SIZE 56
#define HEADER_SIZE sizeof(struct icmphdr)
#define ECHO_SIZE (PAYLOAD_SIZE + HEADER_SIZE)
#define DEFAULT_COUNT 4
#define RECV_TIMEOUT_S 1
#define RECV_SIZE 1500
#define SEND_GAP_NS 1000000L   // пауза между отправками, 1 мс
#define RCV_BUF_PER_TARGET 1024        // оценка skb->truesize одного echo reply
#define RCV_BUF_MAX (4 * 1024 * 1024)  // потолок запроса SO_RCVBUF

typedef enum host_status {
    HOST_NO_REPLY = 0,
    HOST_ALIVE,
    HOST_UNREACHABLE
} host_status;

typedef enum parse_status {
    PARSE_OK = 0,
    PARSE_NO_SLASH,         // '/' не найден — префикс не задан
    PARSE_ADDR_TOO_LONG,    // адресная часть длиннее буфера
    PARSE_BAD_ADDR,         // inet_pton вернул <= 0
    PARSE_BAD_PREFIX,       // префикс не число / мусор в хвосте
    PARSE_PREFIX_RANGE,     // префикс вне [0, 32]
    PARSE_PREFIX_TOO_WIDE,  // валиден, но шире порога sweep
    PARSE_BAD_TIMEOUT,      // не число или вне разумного диапазона
    PARSE_NO_DEFAULT_NET    // CIDR не задан и сеть по умолчанию определить не удалось
} parse_status;

typedef enum defnet_status {
    DEFNET_OK = 0,
    DEFNET_NO_ROUTE_FILE,      // /proc/net/route не открылся — не Linux / сломанный /proc
    DEFNET_NO_DEFAULT_ROUTE,   // таблица есть, маршрута 0.0.0.0/0 в ней нет
    DEFNET_GETIFADDRS_FAILED,  // сбой getifaddrs()
    DEFNET_NO_IFACE_ADDR       // интерфейс найден, но без IPv4-адреса или маски
} defnet_status;

struct target {
    struct in_addr   addr;    // цель; заполняется до рассылки
    struct timespec  t_send;  // момент отправки ЭТОЙ цели; для RTT
    enum host_status status;  // заполняется в фазе сбора
    double           rtt_ms;  // валиден только при HOST_ALIVE
    uint8_t          ttl;     // из IP-заголовка ответа
};

// prototypes
int       parse_cidr(char *input, int count, struct in_addr *ip, int *prefix_len);
int       get_default_network(struct in_addr *ip, int *prefix_len);
int       parse_timeout(char *input, long long *timeout_ms);

int       create_target_list(struct target **list, size_t *count, int prefix_len, struct in_addr in);

int       send_packets(int *fd, const uint16_t id, const size_t count, struct target *list, long long *t_end_send);
int       send_msg(const int fd, const uint8_t *buf, const struct in_addr *addr);
void      setup_buf(uint8_t *buf, const int id, const int seq);
uint16_t  compute_checksum(const uint8_t *data, size_t len);

void      recv_packets(const int fd, const uint16_t id, const size_t count, struct target *list, const long long deadline_ms);
int       answer_to_structs(const uint8_t *answer, const ssize_t answer_size, struct iphdr **ip, ssize_t *ip_len, struct icmphdr **icmp);

void      print_summary(const long long elapsed);

long long now_ms(void);
long long timespec_to_ms(const struct timespec *t);
int       get_int(const char *inp);

#endif // NET_H
