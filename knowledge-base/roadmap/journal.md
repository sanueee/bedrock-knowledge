# Journal — журнал выполненного

**Что сделано: задания + код, reading/theory/mock/ctf.** Сюда пишут скиллы (give-task, vault-write, mock-interview, reading-code, ctf) при закрытии работы. Диспетчер: [00-roadmap.md](00-roadmap.md).

---

## Задачи — writing

> Нумерация **внутри блока** (A-N / B-N / C-N), не сквозная. Файлы: `tasks/block-X/task-<блок><NN>-<name>.md`. Reading/theory/mock/ctf — отдельные последовательности (см. ниже).

### Блок A — Linux internals (C) — закрыт

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

### Блок B — Сетевой стек (C) — активный (6/26)

| # | Задание | Статус | Код |
|---|---------|--------|-----|
| B-1 | [[task-b01-owasp-top10\|OWASP Top 10 (web) теория]] | ✅ выполнено (2026-05-20) | — (reading + vault) |
| B-2 | [[task-b02-tcp-echo\|TCP echo server + client]] | ✅ выполнено (2026-05-17) | `networking/echo_server.c`, `networking/echo_client.c` |
| B-3 | [[task-b03-pcap-sniffer\|pcap sniffer — Ethernet → IP → TCP]] | ✅ выполнено (2026-05-25) | `networking/sniffer.c` |
| B-4 | [[task-b04-getaddrinfo\|getaddrinfo + DNS resolution (resolve)]] | ✅ выполнено (2026-06-05) | `networking/resolve.c` |
| B-5 | [[task-b05-host-probe\|host-probe — banner grab (integration)]] | ✅ выполнено (2026-06-06) | `networking/host_probe.c` |
| B-6 | [[task-b06-nonblocking-io\|non-blocking I/O — nbconnect]] | ✅ выполнено (2026-06-07) | `networking/nbconnect.c` |
| B-7 | [[task-b07-epoll\|epoll — многоклиентский echo на event loop]] | ✅ выполнено (2026-06-18) | `networking/epoll_echo.c` |
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

| # | Блок | Дата | Источник | Цель | Vault |
|---|------|------|----------|------|-------|
| 01 | A | 2026-05-08 | musl libc | popen/pclose/_Fork — fd inheritance, FD_CLOEXEC | [[musl-popen]] |
| 02 | B | 2026-05-21 | Redis (`src/anet.c`) | обёртка над sockets API: симметрия connect/server, гигиена сокета, библиотечная дисциплина ошибок | [[redis-anet]] |

---

## Theory-сессии

| # | Блок | Дата | Темы | Vault |
|---|------|------|------|-------|
| 1 | B (prep) | 2026-05-14 | фундамент перед B-1: syscalls, kernel/user boundary, fd model + refcount, async-signal-safety, copy-on-write, EINTR | [[syscalls-linux]], [[fd-kernel-model]], [[async-signal-safe]], [[virtual-memory-cow]] |

---

## Мок-собесы

| # | Блок | Дата | Слабые места |
|---|------|------|-------------|
| 01 | A | 2026-04-28 | opendir vs readdir, зомби/таблица процессов, sigaction vs signal, pipe EOF |
| 02 | C | 2026-06-04 | receiver'ы (`&mut self`=borrow не владение; `self`=move не shadowing), почему `String` не `Copy` (double free), `match` exhaustiveness=`E0004` compile-time, `v[i]` паника в runtime не compile-time |
| 03 | B | 2026-06-09 | что делает сокет блокирующим (путаница: «таймаут делает blocking»; на деле blocking — дефолт, `SO_RCVTIMEO` лишь ограничивает); исход «hang vs мгновенная сдача» детерминирован режимом, не «что угодно»; `EINTR`=сигнал не «сисколл». Уверенно: recv-исходы, `ip_hl×4`, sockaddr-каст, EAGAIN/EINTR, `SO_ERROR` после POLLOUT |

Детальные записи: `knowledge-base/interviews/interview-NN.md`.
Закрепляющие задания после каждого мок-собеса — оформляются как `task-<блок><NN>-interviewMM-reinforce.md` (нумерация внутри блока) и попадают в раздел "Невыполненные закрепляющие задания" ([00-roadmap.md](00-roadmap.md) → "Текущая позиция"). После выполнения — в "Задачи — writing" под своим блоком.

---

## CTF

| # | Дата | Платформа | Категория | Таск | Главное |
|---|------|-----------|-----------|------|---------|
| 01 | 2026-04-30 | picoCTF | General Skills | [[ctf/picoctf/general-skills/ping-cmd\|ping-cmd]] | command injection через `&&`; space vs shell metacharacter; `execve` over `system` |
| 02 | 2026-05-22 | picoCTF | Forensics | [[ctf/picoctf/forensics/session-02-writeup\|CanYouSee]] | base64 в `Attribution URL` (XMP); подозрительное содержимое поля метаданных = указатель на флаг |
| 03 | 2026-05-22 | picoCTF | Forensics | [[ctf/picoctf/forensics/session-02-writeup\|Verify]] | `sha256sum files/* \| grep <hash>`; `Salted__` = `openssl enc -salt`, не редактором |
| 04 | 2026-05-22 | picoCTF | Forensics | [[ctf/picoctf/forensics/session-02-writeup\|Corrupted file]] | magic bytes JPEG (`FF D8`); `dd conv=notrunc bs=1 count=2` — точечная правка байтов без обрезания файла |
| 05 | 2026-05-22 | picoCTF | Forensics | [[ctf/picoctf/forensics/session-02-writeup\|Secret of the Polyglot]] | polyglot PNG+PDF; `grep -aob '%PDF'` → offset, `dd skip=` извлекает; `Trailer data after IEND` в exiftool = сигнал polyglot |

Цель к концу 2026: **30+ решённых челленджей**. Темп: 1-2 в неделю.
