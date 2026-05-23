---
блок: B (Сетевой стек) → B2
дата: 2026-05-23
статус: в работе
код: networking/sniffer/sniffer.c
vault: [[libpcap]], [[ethernet-frame]]
---

# task-15 — pcap sniffer: Ethernet → IP → TCP

## Задание

Написать утилиту `sniffer`, которая перехватывает live-пакеты на сетевом интерфейсе через **libpcap** и печатает на каждый TCP-пакет одну строку:

```
<ts>  <src_mac> -> <dst_mac>  <src_ip>:<src_port> -> <dst_ip>:<dst_port>  flags=<SYN|ACK|FIN|RST|PSH|URG>  len=<payload_bytes>
```

### Требования

1. **CLI**: `./sniffer <interface>` (например `./sniffer eth0` или `./sniffer lo`). Если интерфейс не указан — `usage` и выход.
2. **BPF-фильтр** на TCP: `pcap_compile` + `pcap_setfilter` со строкой `"tcp"`. Остальное ядро отсеивает до твоего callback'а — не делай руками `if (ethertype == IP && proto == TCP)`, пусть фильтр работает.
3. **Парсинг по слоям**:
   - Ethernet header (`struct ether_header`) → достать MAC src/dst, проверить `ether_type == ETHERTYPE_IP` (на случай не-IP мусора, хотя BPF уже отфильтровал).
   - IP header (`struct ip`) → src/dst IP. **Важно:** длина IP-заголовка переменная, берётся из поля `ip_hl` (в 4-байтных словах) — НЕ хардкодь `sizeof(struct ip)`, считай TCP-заголовок от `(uint8_t*)ip + ip->ip_hl * 4`.
   - TCP header (`struct tcphdr`) → src/dst port (`ntohs`!), флаги. Длина TCP-заголовка тоже переменная: `th_off * 4`.
4. **Payload length** = `ip_len - ip_hl*4 - th_off*4` (всё в host byte order — `ntohs(ip_len)`).
5. **Завершение**: установить обработчик SIGINT, в нём вызвать `pcap_breakloop`, после возврата из `pcap_loop` — `pcap_close`. По выходу напечатать статистику через `pcap_stats` (received / dropped).
6. **Чистая компиляция с `-Wall -Wextra`**.

### Хедеры

- `<pcap/pcap.h>` — `pcap_t`, `pcap_open_live`, `pcap_loop`, `pcap_compile`, `pcap_setfilter`, `pcap_freecode`, `pcap_close`, `pcap_breakloop`, `pcap_stats`, `pcap_geterr`.
- `<netinet/if_ether.h>` — `struct ether_header`, `ETHERTYPE_IP`, `ETH_ALEN`.
- `<netinet/ip.h>` — `struct ip` (поля `ip_hl`, `ip_v`, `ip_len`, `ip_src`, `ip_dst`, `ip_p`).
- `<netinet/tcp.h>` — `struct tcphdr` (поля `th_sport`, `th_dport`, `th_flags`, `th_off`; макросы `TH_SYN`/`TH_ACK`/`TH_FIN`/`TH_RST`/`TH_PSH`/`TH_URG`).
- `<arpa/inet.h>` — `inet_ntop`, `ntohs`, `ntohl`.
- `<signal.h>` — `sigaction`, `SIGINT`.
- `<stdio.h>`, `<stdlib.h>`, `<string.h>`, `<stdint.h>`, `<time.h>`.

### Подготовка окружения

```bash
sudo apt install libpcap-dev
mkdir -p networking/sniffer
```

### Сборка

```bash
gcc -Wall -Wextra -O2 networking/sniffer/sniffer.c -lpcap -o networking/sniffer/sniffer
```

### Запуск

Перехват требует прав:
```bash
sudo ./networking/sniffer/sniffer lo
# либо без sudo, навесив capability один раз:
sudo setcap cap_net_raw,cap_net_admin=eip ./networking/sniffer/sniffer
```

В **другом терминале** сгенерировать TCP-трафик на `lo`:
```bash
# вариант 1 — echo-сервер/клиент с прошлого task-10
./networking/echo_server 5555 &
./networking/echo_client 127.0.0.1 5555
# вариант 2 — curl на любой HTTPS (но это будет на основном интерфейсе, не lo)
curl https://example.com
```

## Подводные камни (на что обратить внимание)

- **Variable-length headers**. Это первое место где IP/TCP — не plain structs. `ip_hl` и `th_off` в **4-байтных словах**, не байтах. Если хардкодишь `+ 20` — поломается на пакетах с опциями.
- **Byte order**. Всё что в шапке многобайтовое (порты, длины, IP-адреса) — в network byte order. `inet_ntop` сам берёт `in_addr` в network order, а вот `ip_len` и `th_sport/th_dport` нужно явно `ntohs`.
- **MAC-форматирование**. `ether_shost`/`ether_dhost` — `uint8_t[6]`, печатай через `printf("%02x:%02x:...", h[0], h[1], ...)`.
- **`pcap_loop` callback signature**: `void cb(u_char *user, const struct pcap_pkthdr *h, const u_char *bytes)`. `h->ts` — timestamp, `h->caplen` — сколько реально захвачено (может быть меньше `h->len` если snaplen маленький).
- **SIGINT внутри `pcap_loop`**. `pcap_loop` блокирует. Обработчик SIGINT должен вызывать `pcap_breakloop(handle)` — это async-signal-safe и кооперативно говорит петле выйти после текущего пакета. Хендл должен быть в глобальной переменной (как `volatile sig_atomic_t` не подходит для указателя, держи просто `static pcap_t *g_handle`).
- **`pcap_open_live` параметры**: `snaplen=65535`, `promisc=1` (для `lo` неважно, для `eth0` — увидишь чужой трафик), `to_ms=1000` (timeout buffer'а).

## Что сохранить в vault (после выполнения, через `vault-write`)

- `topics/networking/libpcap.md` — API: pcap_open_live, BPF filter compile/set, pcap_loop vs pcap_dispatch vs pcap_next, snaplen/promisc/timeout, статистика, breakloop. Глоссарий English.
- `topics/networking/ethernet-frame.md` — Ethernet II frame layout (dst/src MAC, ethertype, payload), IPv4 header layout (variable length via IHL), TCP header layout (variable length via Data Offset), TCP flags. Связь со старым [[proc-net]] (там читали IP/порты уже распарсенными ядром; здесь — байты как они летят по проводу).

## Открытые вопросы (заполнить по ходу)

- *(сюда — что осталось непонятным)*

## Ревью

- *(заполняется через `code-review`)*
