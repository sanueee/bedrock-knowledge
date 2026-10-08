---
тема: Layout пакета — Ethernet → IPv4 → TCP / Packet layering
блок: B — Сетевой стек
дата: 2026-05-25
связано:
  - "[[libpcap]]"
  - "[[tcp-sockets]]"
  - "[[proc-net]]"
  - "[[bitwise-flags]]"
---

# Layout пакета — Ethernet → IPv4 → TCP / Packet layering

## Что это

Бинарный layout трёх инкапсулированных сетевых заголовков как они лежат в захваченном frame'е. Ключевое отличие от parsing'а уже распарсенных структур (`/proc/net/tcp`) — здесь работаешь с сырыми байтами на проводе: переменные длины, network byte order, ручная арифметика смещений.

```
+---------------+--------------------+----------------------+-------------+
| Ethernet 14B  | IPv4 20–60B        | TCP 20–60B           | payload     |
| dst,src,type  | ip_hl*4 = real len | th_off*4 = real len  |             |
+---------------+--------------------+----------------------+-------------+
^               ^                    ^                      ^
bytes           bytes+14             +ip_hl*4 от ip         +th_off*4 от tcp
```

## Ключевые термины (English)

- **Ethernet II frame** — канальный слой, 14 байт заголовка (без CRC, который добавляет NIC)
- **MAC address** — 6 байт, печатается как `xx:xx:xx:xx:xx:xx`
- **ethertype** — поле в Ethernet header, говорит какой протокол выше (0x0800 = IPv4, 0x86DD = IPv6, 0x0806 = ARP)
- **IHL (Internet Header Length)** — поле `ip_hl`, 4 бита, длина IP-заголовка в 4-байтных словах
- **Data Offset** — поле `th_off`, 4 бита, длина TCP-заголовка в 4-байтных словах
- **network byte order (NBO)** — big-endian, стандартный для всех многобайтовых полей в сетевых протоколах
- **host byte order** — порядок байт CPU (на x86 — little-endian)
- **ntohs / ntohl** — network to host, short (16 бит) / long (32 бит)
- **self-describing format** — формат, в котором длина переменной части пишется в сам заголовок
- **IP options** — необязательные поля после фиксированной части IPv4 (timestamp, record route, source routing); редко в современном трафике
- **TCP options** — то же для TCP (MSS, SACK, timestamps, window scale); особенно часто в SYN-пакетах handshake
- **TCP flags** — битовая маска (`th_flags` 8 бит): TH_FIN, TH_SYN, TH_RST, TH_PSH, TH_ACK, TH_URG (+ TH_ECE, TH_CWR для ECN)
- **Ethernet padding** — добивка frame'а нулями до минимума 60 байт (без CRC). Маленький TCP ACK имеет реальные 40 байт IP, но Ethernet frame будет 60.
- **encapsulation** — обёртка: TCP внутри IP, IP внутри Ethernet

## Как работает

### Layout Ethernet header (14 байт)

```
байты 0..5   ether_dhost    6 байт   MAC получателя
байты 6..11  ether_shost    6 байт   MAC отправителя
байты 12..13 ether_type     2 байта  ethertype, NBO
```

```c
const struct ether_header *eth = (const struct ether_header *)bytes;
// eth->ether_dhost — массив uint8_t[6]
// ntohs(eth->ether_type) → ETHERTYPE_IP / ETHERTYPE_IPV6 / ETHERTYPE_ARP
```

### Layout IPv4 header (20..60 байт)

```
байт 0:    бит 0..3  ip_v     версия (4)
           бит 4..7  ip_hl    IHL в 4-байтных словах (5..15)
байт 1:               ip_tos   type of service
байты 2..3            ip_len   полная длина IP-пакета в байтах, NBO
байты 4..5            ip_id
байты 6..7            ip_off
байт 8:               ip_ttl
байт 9:               ip_p     протокол (6 = TCP, 17 = UDP, 1 = ICMP)
байты 10..11          ip_sum   checksum
байты 12..15          ip_src   IPv4 source, NBO
байты 16..19          ip_dst   IPv4 dst, NBO
байты 20..(ip_hl*4-1) IP options (если ip_hl > 5)
```

```c
const struct ip *ip_p = (const struct ip *)(bytes + sizeof(struct ether_header));
// ip_p->ip_hl * 4 — реальная длина IP-заголовка в байтах (обычно 20)
// ntohs(ip_p->ip_len) — полная длина IP-пакета (header + payload)
// inet_ntop(AF_INET, &ip_p->ip_src, str_buf, INET_ADDRSTRLEN) → "1.2.3.4"
```

### Layout TCP header (20..60 байт)

```
байты 0..1   th_sport       source port, NBO
байты 2..3   th_dport       dst port, NBO
байты 4..7   th_seq         sequence number, NBO
байты 8..11  th_ack         ACK number, NBO
байт 12:     бит 0..3 th_off Data Offset в 4-байтных словах (5..15)
             бит 4..7        reserved
байт 13:     th_flags       битовая маска флагов
байты 14..15 th_win         receive window, NBO
байты 16..17 th_sum         checksum
байты 18..19 th_urp         urgent pointer
байты 20..(th_off*4-1)      TCP options
```

```c
const u_char *l4 = (const u_char *)ip_p + ip_p->ip_hl * 4;
const struct tcphdr *tcp_p = (const struct tcphdr *)l4;
uint16_t sport = ntohs(tcp_p->th_sport);
if (tcp_p->th_flags & TH_SYN) { ... }
```

### Формула payload length

```c
int payload_len = ntohs(ip_p->ip_len) - ip_p->ip_hl * 4 - tcp_p->th_off * 4;
```

`int` (не `uint16_t`) — на битом пакете может выйти отрицательное, лучше увидеть `-3` чем underflow в огромное положительное.

### Pointer arithmetic для перехода между слоями

Перед каждым `+ offset` приводи к `u_char *` (или `uint8_t *`):

```c
const u_char *p = (const u_char *)header;
p + N  // == адрес header + N байт
```

Без cast'а: `header + N` сдвигает на `N * sizeof(*header)` — байт-арифметика ломается, получаешь мусор по неправильному адресу.

## Пример

Из `self-practice/networking/pcap-sniffer/sniffer.c`:

```c
if (h->caplen < sizeof(struct ether_header)) return;
const struct ether_header *eth_p = (const struct ether_header *)bytes;
if (ntohs(eth_p->ether_type) != ETHERTYPE_IP) return;

if (h->caplen < sizeof(struct ether_header) + sizeof(struct ip)) return;
const struct ip *ip_p = (const struct ip *)(bytes + sizeof(struct ether_header));

const u_char *l4 = (const u_char *)ip_p + ip_p->ip_hl * 4;
if (h->caplen < (size_t)(l4 - bytes) + sizeof(struct tcphdr)) return;
const struct tcphdr *tcp_p = (const struct tcphdr *)l4;
```

Layered защита: каждая проверка `caplen` покрывает чтение полей следующего шага. 1+2 объединить можно (одна проверка под `ether + sizeof(struct ip)`), 1+2+3 — нет, потому что 3 зависит от `ip_hl`, прочитанного на шаге 2.

## Подводные камни

- **`sizeof(struct ip)` vs `ip_hl * 4`**. `sizeof` = константа 20. Хардкод работает 99% времени, тихо ломается на пакетах с IP options. То же для TCP: `sizeof(struct tcphdr)` = 20, реальный заголовок в SYN-пакетах handshake часто 32..40 байт (MSS + SACK permitted + timestamps + window scale).
- **Pointer arithmetic без cast в `u_char *`**. `(struct ip *)p + 1` = `p + 20`. `(struct ip *)p + ip_hl*4` = `p + 400`. Перед арифметикой по байтам всегда cast в `u_char *`.
- **Network byte order — `ntohs` обязателен** для: `ether_type`, `ip_len`, `ip_id`, `th_sport`, `th_dport`. Для `ip_src`/`ip_dst` cast вручную не нужен — `inet_ntop` сам читает в NBO.
- **TH_PSH (Linux) vs TH_PUSH (macOS)**. Имена флагов в `<netinet/tcp.h>` слегка различаются между BSD и Linux. Cross-platform shim: `#ifdef __APPLE__ #define TH_PSH TH_PUSH #endif`.
- **Ethernet padding**. Минимальный Ethernet frame — 60 байт (без CRC). Маленький TCP ACK без data = 40 байт IP, но в frame'е будет 60 (20 байт padding'а нулями). `caplen - 14 - 20 - 20 = 6` → "6 байт payload", которых физически не было. Поэтому для **смысловой** длины payload использовать `ntohs(ip_len) - ip_hl*4 - th_off*4`, а не вычитание из caplen.
- **th_flags нельзя сравнивать через `==`**. В установленном соединении флаг ACK стоит почти всегда → `tcp->th_flags == TH_SYN` сматчит только чистый SYN (handshake initiator), но не SYN|ACK. Всегда через AND: `if (tcp->th_flags & TH_SYN)`.
- **Layered проверки caplen**. Перед чтением `ip_hl` должна пройти проверка `caplen >= sizeof(ether) + sizeof(struct ip)`. Проверка только под Ethernet header не защищает чтение IP-полей — это UB на коротких пакетах.
- **`ether_type == ETHERTYPE_IP`** после BPF-фильтра `"tcp"` — это "пояс и подтяжки", BPF уже отсеял non-IP. Можно оставить как явную дисциплину парсинга, можно убрать.

## Связанные темы

[[libpcap]] [[tcp-sockets]] [[proc-net]] [[bitwise-flags]]
