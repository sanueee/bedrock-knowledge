# Journal — журнал выполненного

**Что сделано: задания + код, reading/theory/mock/ctf.** Сюда пишут скиллы (give-task, vault-write, mock-interview, reading-code, ctf) при закрытии работы. Диспетчер: [00-roadmap.md](00-roadmap.md).

---

## Задачи — writing

> Нумерация **внутри блока** (A-N / B-N / C-N), не сквозная. Файлы: `sections/<domain>/<name>/task-<блок><NN>-<name>.md` (task-md рядом с кодом). Reading/theory/mock/ctf — отдельные последовательности (см. ниже).

### Блок A — Linux internals + Data Structures (C) — закрыт

**Linux internals:**

| # | Задание | Статус | Код |
|---|---------|--------|-----|
| A-1 | [[task-a01-procinfo\|procinfo]] | ✅ выполнено | `sections/linux/procinfo/procinfo.c` |
| A-2 | [[task-a02-procinfo-top5\|procinfo top5]] | ✅ выполнено | `sections/linux/procinfo-top5/procinfo2.c` |
| A-3 | [[task-a03-fdlist\|fdlist]] | ✅ выполнено | `sections/linux/fdlist/fdlist.c` |
| A-4 | [[task-a04-procfork\|procfork]] | ✅ выполнено | `sections/linux/procfork/procfork.c` |
| A-5 | [[task-a05-signals\|signals]] | ✅ выполнено | `sections/linux/signals/signals.c` |
| A-6 | [[task-a06-pipes\|pipes]] | ✅ выполнено | `sections/linux/pipes/pipechat.c` |
| A-7 | [[task-a07-procnet\|procnet]] | ✅ выполнено | `sections/linux/procnet/procnet.c` |
| A-8 | [[task-a08-pthreads\|pthreads]] | ✅ выполнено | `sections/linux/pthreads/procthreads.c` |
| A-9 | [[task-a09-interview01-reinforce\|interview-01 reinforce]] | ✅ выполнено | `sections/linux/interview01-reinforce/` |

**Data Structures:**

| Структура | Конспект | Статус | Код |
|-----------|----------|--------|-----|
| Массив — частотный подсчёт, row/column-major | [[arrays]] | ✅ | `sections/ds/array-freq/array_freq.c` |
| Запись `struct` — padding/выравнивание | [[records-struct]] | ✅ | `sections/ds/record-ops/record_ops.c` |
| `union` + вариантные записи — type punning | [[union-variant]] | ✅ | `sections/ds/union-variant/union_variant.c` |
| Множество (Set) — битовая маска | [[set]] | ✅ | `sections/ds/bitset/bitset.c` |
| Динамический массив — амортизация, рост | [[dynamic-array]] | ✅ | `sections/ds/dynarray/dynarray.c` |
| Линейный список — reverse, `Node**` | [[linked-list]] | ✅ | `sections/ds/list/list.c` |
| Стек / очередь / дека | [[stack-queue-deque]] | ✅ | `sections/ds/stack-queue-deque/stack_queue_deque.c` |
| BST — insert/search/delete(3 случая)/free | [[trees-bst]], [[recursion-bst-c]] | ✅ | `sections/ds/bst/bst.c` |
| AVL — повороты + единый rebalance | [[avl]] | ✅ | `sections/ds/avl/avl.c` |
| Splay — zig/zig-zig/zig-zag + insert | [[splay]] | ✅ | `sections/ds/splay/splay.c` |
| B-дерево — split/merge, borrow | [[btree]] | ✅ | `sections/ds/btree/btree.c` |

### Блок B — Сетевой стек (C) — активный (18/26)

| # | Задание | Статус | Код |
|---|---------|--------|-----|
| B-1 | [[task-b01-owasp-top10\|OWASP Top 10 (web) теория]] | ✅ выполнено (2026-05-20) | — (reading + vault) |
| B-2 | [[task-b02-tcp-echo\|TCP echo server + client]] | ✅ выполнено (2026-05-17) | `sections/networking/tcp-echo/echo_server.c`, `sections/networking/tcp-echo/echo_client.c` |
| B-3 | [[task-b03-pcap-sniffer\|pcap sniffer — Ethernet → IP → TCP]] | ✅ выполнено (2026-05-25) | `sections/networking/pcap-sniffer/sniffer.c` |
| B-4 | [[task-b04-getaddrinfo\|getaddrinfo + DNS resolution (resolve)]] | ✅ выполнено (2026-06-05) | `sections/networking/getaddrinfo/resolve.c` |
| B-5 | [[task-b05-host-probe\|host-probe — banner grab (integration)]] | ✅ выполнено (2026-06-06) | `sections/networking/host-probe/host_probe.c` |
| B-6 | [[task-b06-nonblocking-io\|non-blocking I/O — nbconnect]] | ✅ выполнено (2026-06-07) | `sections/networking/nonblocking-io/nbconnect.c` |
| B-7 | [[task-b07-epoll\|epoll — многоклиентский echo на event loop]] | ✅ выполнено (2026-06-18) | `sections/networking/epoll/epoll_echo.c` |
| B-8 | [[task-b08-connect-timeout\|connect-with-timeout — примитив connect-scan]] | ✅ выполнено (2026-07-02) | `sections/networking/connect-timeout/connect_timeout.c` |
| B-9 | [[task-b09-udp-echo\|UDP echo — recvfrom/sendto, датаграммные границы, потеря пакета]] | ✅ выполнено (2026-07-03) | `sections/networking/udp-echo/udp_echo.c` |
| B-10 | [[task-b10-port-scan-v1\|port-scan v1 — connect-scan диапазона, bounded concurrency (integration)]] | ✅ выполнено (2026-07-04) | `sections/networking/port-scan-v1/portscan.c` |
| B-11 | [[task-b11-http-basics\|HTTP basics — HTTP/1.1 клиент, framing (Content-Length/chunked/close), парсинг]] | ✅ выполнено (2026-07-05) | `sections/networking/http-basics/httpget.c` |
| B-12 | [[task-b12-dns-query\|DNS-резолвер вручную — A-record поверх UDP, wire format, компрессия имён]] | ✅ выполнено (2026-07-07) | `sections/networking/dns-query/dnsquery.c` |
| B-13 | [[task-b13-service-fingerprint\|Service fingerprinting — библиотека проб SSH/HTTP/SMTP/FTP, server/client-first, memmem-матчинг]] | ✅ выполнено (2026-07-12) | `sections/networking/service-fingerprint/fingerprint.c` |
| B-14 | [[task-b14-tls-handshake\|TLS handshake observational — Wireshark на HTTPS, ClientHello/ServerHello/Certificate, ECDHE, SNI/ALPN, decrypt через SSLKEYLOGFILE]] | ✅ выполнено (2026-07-15) | — (observational + `topics/networking/tls.md`) |
| B-15 | [[task-b15-scanner-v2\|scanner v2 — discovery диапазона + идентификация сервиса + извлечение детали (integration)]] | ✅ выполнено (2026-07-18) | `sections/networking/scanner-v2/scanner-v2.c` |
| B-16 | [[task-b16-iface-enum\|Interface enumeration — getifaddrs + /proc/net/route, маски→CIDR, default gateway, endianness]] | ✅ выполнено (2026-07-19) | `sections/networking/iface-enum/ifenum.c` |
| B-17 | [[task-b17-raw-socket-caps\|Raw sockets и capabilities — CAP_NET_RAW, minimum capability surface, setcap +ep, libcap-ng]] | ✅ выполнено (2026-07-20) | `sections/networking/raw-socket-caps/rawcap.c` |
| B-18 | [[task-b18-internet-checksum\|Internet checksum — RFC 1071 one's complement, IP/TCP заголовки, pseudo-header, byte order на записи check]] | ✅ выполнено (2026-07-27) | `sections/networking/internet-checksum/checksum.c` |
| B-rf | [[task-b-interview03-reinforce\|sockmode_demo — матрица blocking/non-blocking (Interview 03 reinforce)]] | ✅ выполнено (2026-06-09) | `sections/networking/interview03-reinforce/sockmode_demo.c` |

### Блок C — Rust (параллельный трек) — активный (5/?)

| # | Задание | Статус | Код |
|---|---------|--------|-----|
| C-1 | [[task-c01-rust-ch1-2\|Rust: toolchain + hello_world + Guessing Game]] | ✅ выполнено (2026-05-18) | `sections/rust/rust-ch1-2/hello_world/`, `sections/rust/rust-ch1-2/guessing_game/` |
| C-2 | [[task-c02-rust-ch3-4\|Rust ch.3–4 + word_tools (slices/ownership)]] | ✅ выполнено (2026-05-18) | `sections/rust/rust-ch3-4/word_tools/src/main.rs` |
| C-3 | [[task-c03-rust-ch5-structs\|Rust ch.5 — structs + methods (rectangles)]] | ✅ выполнено (2026-05-23) | `sections/rust/rust-ch5-structs/rectangles/src/main.rs` |
| C-4 | [[task-c04-rust-ch6-enums-match\|Rust ch.6 — enums + match + Option (scan_report)]] | ✅ выполнено (2026-06-03) | `sections/rust/rust-ch6-enums-match/scan_report/src/main.rs` |
| C-5 | [[task-c05-rust-ch7-8-modules-collections\|Rust ch.7–8 — modules + collections (scan_aggregator)]] | ✅ выполнено (2026-06-04) | `sections/rust/rust-ch7-8-modules-collections/scan_aggregator/src/` |

---

## Reading-сессии

Детальные записи: `knowledge-base/topics/reading/**.md`
---

## Мок-собесы

Детальные записи: `knowledge-base/interviews/interview-NN.md`.

---

## CTF

| # | Дата | Платформа | Категория | Таск | Главное |
|---|------|-----------|-----------|------|---------|
| 01 | 2026-04-30 | picoCTF | General Skills | [[ctf/picoctf/general-skills/ping-cmd\|ping-cmd]] | command injection через `&&`; space vs shell metacharacter; `execve` over `system` |
| 02 | 2026-05-22 | picoCTF | Forensics | [[ctf/picoctf/forensics/session-02-writeup\|CanYouSee]] | base64 в `Attribution URL` (XMP); подозрительное содержимое поля метаданных = указатель на флаг |
| 03 | 2026-05-22 | picoCTF | Forensics | [[ctf/picoctf/forensics/session-02-writeup\|Verify]] | `sha256sum files/* \| grep <hash>`; `Salted__` = `openssl enc -salt`, не редактором |
| 04 | 2026-05-22 | picoCTF | Forensics | [[ctf/picoctf/forensics/session-02-writeup\|Corrupted file]] | magic bytes JPEG (`FF D8`); `dd conv=notrunc bs=1 count=2` — точечная правка байтов без обрезания файла |
| 05 | 2026-05-22 | picoCTF | Forensics | [[ctf/picoctf/forensics/session-02-writeup\|Secret of the Polyglot]] | polyglot PNG+PDF; `grep -aob '%PDF'` → offset, `dd skip=` извлекает; `Trailer data after IEND` в exiftool = сигнал polyglot |
