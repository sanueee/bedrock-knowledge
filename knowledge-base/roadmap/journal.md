# Journal — журнал выполненного

**Что сделано: задания + код, reading/theory/mock/ctf.** Сюда пишут скиллы (give-task, vault-write, mock-interview, reading-code, ctf) при закрытии работы. Диспетчер: [00-roadmap.md](00-roadmap.md).

---

## Задачи — writing

> Нумерация **внутри блока** (A-N / B-N / C-N), не сквозная. Файлы: `tasks/block-X/task-<блок><NN>-<name>.md`. Reading/theory/mock/ctf — отдельные последовательности (см. ниже).

### Блок A — Linux internals + Data Structures (C) — закрыт

**Linux internals:**

| # | Задание | Статус | Код |
|---|---------|--------|-----|
| A-1 | [[task-a01-procinfo\|procinfo]] | ✅ выполнено | `linux/proc/procinfo.c` |
| A-2 | [[task-a02-procinfo-top5\|procinfo top5]] | ✅ выполнено | `linux/proc/procinfo2.c` |
| A-3 | [[task-a03-fdlist\|fdlist]] | ✅ выполнено | `linux/fd/fdlist.c` |
| A-4 | [[task-a04-procfork\|procfork]] | ✅ выполнено | `linux/proc/procfork.c` |
| A-5 | [[task-a05-signals\|signals]] | ✅ выполнено | `linux/proc/signals.c` |
| A-6 | [[task-a06-pipes\|pipes]] | ✅ выполнено | `linux/proc/pipechat.c` |
| A-7 | [[task-a07-procnet\|procnet]] | ✅ выполнено | `linux/proc/procnet.c` |
| A-8 | [[task-a08-pthreads\|pthreads]] | ✅ выполнено | `linux/proc/procthreads.c` |
| A-9 | [[task-a09-interview01-reinforce\|interview-01 reinforce]] | ✅ выполнено | `linux/reinforce/` |

**Data Structures:**

| Структура | Конспект | Статус | Код |
|-----------|----------|--------|-----|
| Массив — частотный подсчёт, row/column-major | [[05-arrays]] | ✅ | `ds/array_freq.c` |
| Запись `struct` — padding/выравнивание | [[06-records-struct]] | ✅ | `ds/record_ops.c` |
| `union` + вариантные записи — type punning | [[07-union-variant]] | ✅ | `ds/union_variant.c` |
| Множество (Set) — битовая маска | [[08-set]] | ✅ | `ds/bitset.c` |
| Динамический массив — амортизация, рост | [[11-dynamic-array]] | ✅ | `ds/dynarray.c` |
| Линейный список — reverse, `Node**` | [[12-linked-list]] | ✅ | `ds/list.c` |
| Стек / очередь / дека | [[13-stack-queue-deque]] | ✅ | `ds/stack_queue_deque.c` |
| BST — insert/search/delete(3 случая)/free | [[14-trees-bst]], [[15-recursion-bst-c]] | ✅ | `ds/bst.c` |
| AVL — повороты + единый rebalance | [[16-avl]] | ✅ | `ds/avl.c` |
| Splay — zig/zig-zig/zig-zag + insert | [[17-splay]] | ✅ | `ds/splay.c` |
| B-дерево — split/merge, borrow | [[18-btree]] | ✅ | `ds/btree.c` |

### Блок B — Сетевой стек (C) — активный (13/26)

| # | Задание | Статус | Код |
|---|---------|--------|-----|
| B-1 | [[task-b01-owasp-top10\|OWASP Top 10 (web) теория]] | ✅ выполнено (2026-05-20) | — (reading + vault) |
| B-2 | [[task-b02-tcp-echo\|TCP echo server + client]] | ✅ выполнено (2026-05-17) | `networking/echo_server.c`, `networking/echo_client.c` |
| B-3 | [[task-b03-pcap-sniffer\|pcap sniffer — Ethernet → IP → TCP]] | ✅ выполнено (2026-05-25) | `networking/sniffer.c` |
| B-4 | [[task-b04-getaddrinfo\|getaddrinfo + DNS resolution (resolve)]] | ✅ выполнено (2026-06-05) | `networking/resolve.c` |
| B-5 | [[task-b05-host-probe\|host-probe — banner grab (integration)]] | ✅ выполнено (2026-06-06) | `networking/host_probe.c` |
| B-6 | [[task-b06-nonblocking-io\|non-blocking I/O — nbconnect]] | ✅ выполнено (2026-06-07) | `networking/nbconnect.c` |
| B-7 | [[task-b07-epoll\|epoll — многоклиентский echo на event loop]] | ✅ выполнено (2026-06-18) | `networking/epoll_echo.c` |
| B-8 | [[task-b08-connect-timeout\|connect-with-timeout — примитив connect-scan]] | ✅ выполнено (2026-07-02) | `networking/connect_timeout.c` |
| B-9 | [[task-b09-udp-echo\|UDP echo — recvfrom/sendto, датаграммные границы, потеря пакета]] | ✅ выполнено (2026-07-03) | `networking/udp_echo.c` |
| B-10 | [[task-b10-port-scan-v1\|port-scan v1 — connect-scan диапазона, bounded concurrency (integration)]] | ✅ выполнено (2026-07-04) | `networking/portscan.c` |
| B-11 | [[task-b11-http-basics\|HTTP basics — HTTP/1.1 клиент, framing (Content-Length/chunked/close), парсинг]] | ✅ выполнено (2026-07-05) | `networking/httpget.c` |
| B-12 | [[task-b12-dns-query\|DNS-резолвер вручную — A-record поверх UDP, wire format, компрессия имён]] | ✅ выполнено (2026-07-07) | `networking/dnsquery.c` |
| B-13 | [[task-b13-service-fingerprint\|Service fingerprinting — библиотека проб SSH/HTTP/SMTP/FTP, server/client-first, memmem-матчинг]] | ✅ выполнено (2026-07-12) | `networking/fingerprint.c` |
| B-rf | [[task-b-interview03-reinforce\|sockmode_demo — матрица blocking/non-blocking (Interview 03 reinforce)]] | ✅ выполнено (2026-06-09) | `networking/sockmode_demo.c` |

### Блок C — Rust (параллельный трек) — активный (5/?)

| # | Задание | Статус | Код |
|---|---------|--------|-----|
| C-1 | [[task-c01-rust-ch1-2\|Rust: toolchain + hello_world + Guessing Game]] | ✅ выполнено (2026-05-18) | `rust/hello_world/`, `rust/guessing_game/` |
| C-2 | [[task-c02-rust-ch3-4\|Rust ch.3–4 + word_tools (slices/ownership)]] | ✅ выполнено (2026-05-18) | `rust/word_tools/src/main.rs` |
| C-3 | [[task-c03-rust-ch5-structs\|Rust ch.5 — structs + methods (rectangles)]] | ✅ выполнено (2026-05-23) | `rust/rectangles/src/main.rs` |
| C-4 | [[task-c04-rust-ch6-enums-match\|Rust ch.6 — enums + match + Option (scan_report)]] | ✅ выполнено (2026-06-03) | `rust/scan_report/src/main.rs` |
| C-5 | [[task-c05-rust-ch7-8-modules-collections\|Rust ch.7–8 — modules + collections (scan_aggregator)]] | ✅ выполнено (2026-06-04) | `rust/scan_aggregator/src/` |

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
