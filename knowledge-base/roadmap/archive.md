# Archive — пройденные темы и индекс знаний

**Читается редко** — при оценке знаний, пересмотре стратегии, проверке «проходили ли уже X». Не грузится в контекст каждой сессии. Диспетчер: [00-roadmap.md](00-roadmap.md).

> «Пройдено» = работал с темой в прошлых сессиях (писал код, разбирал, проходил мок-собес). Это **не** означает мастерство — знания выветриваются. Перед заданием по старой теме — перечитать конспект (см. [[wikilink]] ниже), а не полагаться на эту запись.
>
> Когда блок закрывается — его детальный дамп тем переезжает сюда из диспетчера.

---

## Фундамент (theory 2026-05-14)

syscalls + kernel/user boundary, fd kernel model + refcount, async-signal-safety, virtual memory + copy-on-write, EINTR семантика.

---

## Блок A — Linux internals + Data Structures (C) — закрыт

Две части одного фундаментального C-блока. Темы Linux добираются в блоке G по новой структуре; темы СД остаются базой под основной вектор.

### Часть 1 — Linux internals

Закрыт задним числом по старой схеме (8 атомарных + Interview 01 reinforce).

**Пройденные темы:** /proc filesystem, directory traversal, file descriptors + readlink, snprintf/qsort, error handling, fork/exec/wait, signals (sigaction/SIGINT/SIGTERM/pause), pipes/IPC (pipe/dup2/read/write), /proc/net/tcp parsing (sscanf, bitwise ops, little-endian IP), pthreads (pthread_create/join, mutex, data race), interview-01 reinforce (opendir/readdir errors, getppid vs fork return, WIFEXITED/WEXITSTATUS, async-signal-safety, volatile sig_atomic_t, SA_RESTART, pipe write-end counter, char* vs char[]).

### Часть 2 — Data Structures (экзамен 1 курса СПбПУ, 2026-06)

Полный курс структур данных на C под экзамен (10 дней, 2026-06-10 … 2026-06-20). **Хаб со статусом, картой тем и журналом пробников — `topics/ds/00-index.md`** (primary source), конспекты — `topics/ds/NN-*.md`, код — `ds/`. «Пройдено ≠ мастерство» — перед опорой на тему перечитать конспект в `topics/ds/`.

---

## Блок B — Сетевой стек (C) — активный (детальный дамп)

> Блок активен — дамп живёт здесь, диспетчер держит только позицию/следующий шаг/слабые места.

**Пройденные темы** (суть в 1–2 предложениях; детали — в конспектах по ссылкам):

- **B-10** (интеграционное, первое в блоке) — port-scan v1 (`portscan.c`, синтез B-6/B-7/B-8): connect-scan диапазона портов. **Bounded concurrency** — окно фиксированных слотов (256), fd→slot через `epoll_event.data.u32` (не `port_of_fd[]`); per-socket дедлайны на `CLOCK_MONOTONIC`; три вердикта open/closed/filtered на диапазоне. Ключевые уроки (4 итерации `/check`): `epoll_wait` ждёт **относительный** таймаут (`min_deadline - now`, кламп ≥0), не абсолютный момент; условие выхода цикла = порты **ИЛИ** активные пробы (иначе filtered теряется, loopback прячет баг); счётчик in-flight парен занятию слота (лишний `active--` в `CONNECT_ERROR` = рассинхрон). → [[port-scan-scaling]].
- **B-9** — UDP sockets (`udp_echo.c`): `SOCK_DGRAM`, connectionless (без `listen`/`accept`/`connect`), `recvfrom`/`sendto` с явным адресом peer'а. Три свойства: датаграммные границы (усечение вместо partial read при малом буфере), потеря пакета как штатное событие (`SO_RCVTIMEO` → `EAGAIN`), эфемерный порт клиента. Security: `%s` по `recvfrom`-буферу = OOB-read (нет `'\0'`) → печать `%.*s` по `n`. → [[udp-sockets]].
- **B-8** — Connect-with-timeout (`connect_timeout.c`, синтез B-6+B-7): non-blocking `connect` → `epoll` ждёт `EPOLLOUT` с таймаутом → `getsockopt(SO_ERROR)` = вердикт (open/closed/filtered). Ключ: readiness ≠ correctness (EPOLLOUT срабатывает и на RST). → [[connect-timeout-scan]].
- **B-7** — Multiplexing: epoll (`epoll_echo.c`, многоклиентский echo): однопоточный event loop, диспетчер по битовой маске `events`, EPOLLOUT-backlog + back-pressure через readiness, чистый выход по SIGINT без `SA_RESTART`. → [[event-loop-epoll]].
- **B-6** — non-blocking I/O (`nbconnect.c`): режим сокета решает, кто ждёт (`O_NONBLOCK` → `EAGAIN`); non-blocking `connect` → `EINPROGRESS` → `poll(POLLOUT)` → `SO_ERROR`; «не висеть вечно» даёт таймаут в poll, не сам poll. → [[nonblocking-poll]].
- **B-5** (интеграционное) — host-probe / banner grab (`host_probe.c`, синтез B-2/B-3/B-4): read timeout `SO_RCVTIMEO`, различение `EAGAIN` vs `EINTR` на recv, перебор адресов `getaddrinfo`, lifetime после `freeaddrinfo`. → [[tcp-sockets]], [[stdio-buffering]].
- **B-4** — getaddrinfo + DNS (`resolve.c`): резолв «имя + service → список addrinfo», извлечение адреса из generic `sockaddr *` кастом по `ai_family`, `inet_ntop`/`ntohs`, `gai_strerror` (свой namespace), `freeaddrinfo`. → [[getaddrinfo]], [[posix-naming]].
- **B-3** — libpcap sniffer (`sniffer.c`): pcap lifecycle + BPF-фильтр в ядре, Ethernet/IPv4/TCP layout байт за байтом, переменная длина заголовков (`ip_hl*4`/`th_off*4`), NBO, pointer arithmetic через `u_char *`. → [[libpcap]], [[ethernet-frame]].
- **B-2** — TCP-сокеты (`echo_server.c`/`echo_client.c`): socket/bind/listen/accept/connect/send/recv, `sockaddr_in`+htons, three-way recv (0/-1/>0), partial read, EPIPE/ECONNRESET, SIGPIPE через `SIG_IGN`. → [[tcp-sockets]].
- **B-1** — OWASP Top 10 web (2021) теория: 10 категорий (authn/authz, prepared statement, XSS, SSRF, IDOR, password hashing, supply chain); принципы defense in depth, структурные защиты > текстовые. → [[owasp-top10]].

---

## Блок C — Rust (параллельный трек) — активный (детальный дамп)

**Пройденные темы** (суть в 1–2 предложениях; детали — в конспектах по ссылкам):

- **C-1** (главы 1–2) — Rust toolchain (rustup/rustc/cargo, Cargo.toml/lock, edition, crate binary vs library) + hello_world, guessing_game. → [[toolchain]].
- **C-2** (главы 3–4) — типы, shadowing, statements vs expressions, control flow/ranges; ownership (move vs Copy), borrow (`&T`/`&mut T` — XOR), `String` vs `&str`, slices, dangling как compile error. Видел реальные сообщения borrow checker'а (E0499/E0502/E0382/E0515). → [[types-control-flow]], [[ownership]].
- **C-3** (глава 5) — struct (named/tuple/unit), `impl`, три формы receiver (`&self`/`&mut self`/`self`), associated function vs method, `Self`, automatic referencing, `#[derive(Debug)]`. → [[structs-methods]].
- **C-4** (глава 6) — enum с данными, `match` как выражение (exhaustive), `Option<T>` вместо null, `if let`, match ergonomics, Drop/RAII; написал `scan_report`. Слабое место: match как выражение зашёл тяжело. → [[enums-match]].
- **C-5** (главы 7–8) — модульная система (package⊃crate⊃module⊃item, пути `crate`/`super`/`self`, `pub`-видимость, `use`, prelude) + collections (`Vec`/`String`/`HashMap`, `entry` API); написал `scan_aggregator`. Слабые места (Interview 02): ownership-терминология, уровни package/crate/module. → [[modules]], [[collections]].

---

## Индекс тем (knowledge map)

Карта конспектов в `topics/`. Wikilinks резолвятся по имени независимо от папки.

### Темы — Linux

- [[proc-filesystem]] — файловая система /proc
- [[fork-linux]] — создание процессов, fork/waitpid
- [[signals-linux]] — сигналы, sigaction, SIGINT/SIGTERM
- [[pipes-linux]] — межпроцессное взаимодействие через pipe
- [[proc-net]] — чтение /proc/net/tcp, парсинг TCP-соединений
- [[pthreads-linux]] — потоки, pthread_create/join, мьютексы
- [[symlinks-linux]] — символические ссылки, readlink
- [[anon-inode]] — анонимные inode: epoll, signalfd, timerfd
- [[ssh-basics]] — основы SSH
- [[syscalls-linux]] — системные вызовы, kernel/user boundary
- [[fd-kernel-model]] — file descriptor table, struct file, refcount
- [[async-signal-safe]] — какие функции safe в обработчиках сигналов
- [[virtual-memory-cow]] — виртуальная память и copy-on-write при fork

### Темы — Rust

- [[toolchain]] — rustup, rustc, cargo, Cargo.toml/lock, crate, edition, crates.io
- [[guessing-game-notes]] — открытые вопросы после главы 2 The Book (закрыто 1,2,3,5,6,7; открыто 4,8)
- [[types-control-flow]] — глава 3: immutable by default, shadowing, типы, statements vs expressions, ranges
- [[ownership]] — глава 4: move/copy, `&T`/`&mut T`, slices, dangling, реальные сообщения borrow checker'а, Drop/RAII (освобождение по владельцу)
- [[structs-methods]] — глава 5: struct (named/tuple/unit), `impl`, `&self`/`&mut self`/`self`, associated function vs method, `Self`, automatic referencing, `#[derive(Debug)]` + `{:?}`/`{:#?}`/`dbg!`
- [[enums-match]] — глава 6: enum с данными, `match` как выражение (exhaustive), `Option<T>` вместо null, `if let`, match ergonomics
- [[modules]] — глава 7: package ⊃ crate ⊃ module ⊃ item, пути `crate`/`super`/`self`, `pub`-видимость + `E0603`, `use` как ярлык (не `#include`), prelude
- [[collections]] — глава 8: `Vec`/`String`/`HashMap`, `entry` API (`or_insert_with`/`or_insert`), `*`-разыменование счётчика, `v[i]` vs `v.get`, итерация borrow vs move

### Темы — Networking

- [[tcp-sockets]] — TCP-сокеты, socket/bind/listen/accept/connect, partial read, EPIPE, SIGPIPE защита
- [[nonblocking-poll]] — blocking vs non-blocking сокеты + poll: матрица 2×2, три роли (режим/poll/timeout/recv), EINPROGRESS connect, SO_ERROR (прицельно под слабое место B-6)
- [[event-loop-epoll]] — event loop model, epoll, non-blocking I/O, readiness ≠ correctness
- [[connect-timeout-scan]] — connect-with-timeout, примитив connect-scan (B-6+B-7): три исхода open/closed/filtered, SO_ERROR = вердикт, синхронный vs async refused, Docker Desktop NAT врёт
- [[udp-sockets]] — UDP sockets: recvfrom/sendto, connectionless, датаграммные границы (усечение), потеря пакета = SO_RCVTIMEO/EAGAIN, %s по recvfrom-буферу = OOB-read
- [[port-scan-scaling]] — масштабирование connect-scan: bounded concurrency, окно проб, fd→slot через data.u32, relative epoll timeout, liveness-условие цикла (порты ИЛИ активные пробы), счётчик in-flight
- [[nagle-tcp-nodelay]] — алгоритм Нагла, TCP_NODELAY, когда выключать
- [[getaddrinfo]] — resolve, addrinfo linked list, gai_strerror, паттерн "свой namespace ошибок"
- [[libpcap]] — захват пакетов из user space, BPF фильтр в ядре, pcap lifecycle, caplen vs len, breakloop
- [[ethernet-frame]] — layout Ethernet/IPv4/TCP в памяти, ip_hl*4 / th_off*4, NBO, pointer arithmetic через u_char *, payload length

### Темы — Security / CTF

- [[command-injection]] — command injection в picoCTF ping-cmd, защита через `execve`
- [[owasp-top10]] — OWASP Top 10 (web, 2021): 10 категорий мышления, prepared statement / XSS / SSRF / IDOR глубоко
- [[session-02-writeup]] — Forensics session-02: 4 таска (метаданные, sha256sum+openssl, magic bytes JPEG, polyglot PNG+PDF); таблица инструментов forensics (`exiftool`, `xxd`, `dd conv=notrunc`, `grep -aob`)

### Темы — C

- [[file-io-c]] — fopen/fgets/fclose vs open/read
- [[directory-traversal-c]] — opendir/readdir/closedir
- [[string-formatting-c]] — snprintf, sscanf, strncmp
- [[error-handling-c]] — обработка ошибок, errno, perror
- [[sorting-c]] — qsort, компаратор
- [[bitwise-flags]] — битовые флаги, идиомы (`|=`, `&= ~`, `!!`), file status vs descriptor flags
- [[posix-naming]] — словарь сокращений в именах типов/функций (`in`/`sa`/`ai`/`sin6`, зоопарк sockaddr*, width vs precision, byte order `n`/`h`)
- [[stdio-buffering]] — буферизация stdout (line/full/unbuffered) vs stderr, переплетение строк под pipe, fflush, потеря вывода при краше

### Темы — Reading (разборы чужого кода)

- [[musl-popen]] — musl libc popen/pclose/_Fork, fd inheritance, FD_CLOEXEC
- [[redis-anet]] — Redis обёртка над sockets API: симметрия connect/server, гигиена сокета, паттерн goto error
