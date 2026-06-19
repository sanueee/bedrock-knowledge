# Archive — пройденные темы и индекс знаний

**Читается редко** — при оценке знаний, пересмотре стратегии, проверке «проходили ли уже X». Не грузится в контекст каждой сессии. Диспетчер: [00-roadmap.md](00-roadmap.md).

> «Пройдено» = работал с темой в прошлых сессиях (писал код, разбирал, проходил мок-собес). Это **не** означает мастерство — знания выветриваются. Перед заданием по старой теме — перечитать конспект (см. [[wikilink]] ниже), а не полагаться на эту запись.
>
> Когда блок закрывается — его детальный дамп тем переезжает сюда из диспетчера.

---

## Фундамент (theory 2026-05-14)

syscalls + kernel/user boundary, fd kernel model + refcount, async-signal-safety, virtual memory + copy-on-write, EINTR семантика.

---

## Блок A — Linux internals (C) — закрыт

Закрыт задним числом по старой схеме (8 атомарных + Interview 01 reinforce). Темы добираются в блоке G по новой структуре.

**Пройденные темы:** /proc filesystem, directory traversal, file descriptors + readlink, snprintf/qsort, error handling, fork/exec/wait, signals (sigaction/SIGINT/SIGTERM/pause), pipes/IPC (pipe/dup2/read/write), /proc/net/tcp parsing (sscanf, bitwise ops, little-endian IP), pthreads (pthread_create/join, mutex, data race), interview-01 reinforce (opendir/readdir errors, getppid vs fork return, WIFEXITED/WEXITSTATUS, async-signal-safety, volatile sig_atomic_t, SA_RESTART, pipe write-end counter, char* vs char[]).

---

## Блок B — Сетевой стек (C) — активный (детальный дамп)

> Блок активен — дамп живёт здесь, диспетчер держит только позицию/следующий шаг/слабые места.

**Пройденные темы:** B-7 — Multiplexing: epoll (`epoll_echo.c`, многоклиентский echo, LT): однопоточный event loop (`epoll_create1` → ADD на `EPOLLIN` → цикл `epoll_wait` → диспетчер); **диспетчер по битовой маске `events[i].events`** (`EPOLLIN/EPOLLOUT/EPOLLERR/EPOLLHUP`) — «пришло событие» ≠ «читать»; **accept-цикл и recv до `EAGAIN`**; **EPOLLOUT-backlog** — недописанный non-blocking `send` (partial / `EAGAIN` = буфер ядра полон) нельзя крутить на месте (busy-wait), остаток буферизуется per-client (`conn_t{buf,len,sent}`), `arm(EPOLLIN|EPOLLOUT)`, дослать по `EPOLLOUT`, опустошил → `arm(EPOLLIN)` (back-pressure через readiness); `arm()` = обёртка `epoll_ctl(EPOLL_CTL_MOD)`; **flow-control размен** — читать новую порцию только когда прошлая отправлена (`c->len==0`), иначе backlog растёт без границы, платя узлом «drain до EAGAIN»; **снятие клиента** — порядок строго `EPOLL_CTL_DEL`→`close` (close раньше = дыра на reuse fd), `EPOLL_CTL_DEL` принимает `NULL`; **reuse fd** — `conns[fd]` по номеру fd (статический массив + guard `cfd>=FD_MAX`), при close сброс `len=sent=0` (иначе stale state); **чистый выход по SIGINT** — `sigaction` **без `SA_RESTART`** (Ctrl+C рвёт `epoll_wait` с `EINTR` → `while(!g_stop)` → `close`), хендлер только взводит `volatile sig_atomic_t`. Грабли: `#include <cerrno>` (C++) на C = fatal + каскад «epoll undeclared»; epoll — Linux-only (прогон в Docker `gcc`, на macOS не собрать). Прогон: сборка чистая `-Wall -Wextra`, 3 одновременных клиента — независимый echo. Конспект: [[event-loop-epoll]] (дополнен). Код: `networking/epoll_echo.c`. B-6 — non-blocking I/O (`nbconnect`): **режим сокета решает, кто ждёт** — blocking (ждёт ядро в syscall) vs non-blocking `O_NONBLOCK` (syscall не спит, «не готово» = `-1`+`EAGAIN`/`EWOULDBLOCK`); установка флага `fcntl(F_GETFL)` → `| O_NONBLOCK` → `F_SETFL` (не затереть прочие file status flags); **non-blocking `connect` = fire-and-forget** → `-1`+`EINPROGRESS` (handshake в фоне, НЕ повторять — иначе `EALREADY`); `EINPROGRESS` ≠ `EINTR` (в процессе vs прервано-до-работы); **два канала ошибок connect** — синхронный `errno` (вызов вернул `-1` сейчас) vs асинхронный `SO_ERROR` (провал в фоне, когда syscall не активен → ядро паркует ошибку на сокете, read-and-clear); связка завершения connect: `poll(POLLOUT)` (ждать готовности) → `getsockopt(SO_ERROR)` (узнать результат — **readiness ≠ correctness**, POLLOUT срабатывает и на успех и на провал); **три роли при non-blocking I/O** — режим (не спать) / `poll` (где ждать) / `timeout` у poll (сколько ждать) / `recv`-цикл (чем вычерпывать до `EAGAIN`/`0`); матрица 2×2 (сокет × poll): убрать `poll` на non-blocking → **не зависание, а преждевременная сдача** по `EAGAIN` (зависание — свойство blocking); «не висеть вечно» даёт **таймаут в `poll`**, не `poll` сам. Структурный урок: цикл не должен делать две работы — развязка на 2 фазы (найти подключённый fd / читать баннер). Конспект: [[nonblocking-poll]] (новый, прицельно под слабое место). Код: `networking/nbconnect.c`. B-5 (интеграционное) — `host-probe` (banner grab): синтез B-2/B-3/B-4. **Read timeout `SO_RCVTIMEO`** (иначе recv на молчащем сервере висит вечно — клинч HTTP :80, где сервер ждёт запрос); **различение двух `-1` на recv: `EAGAIN`/`EWOULDBLOCK`(таймаут истёк → принять как «баннера нет») vs `EINTR`(сигнал → retry)** — противоположные реакции, идиома «съесть EINTR в `do/while` до разбора веток»; перебор адресов `getaddrinfo` с попыткой `connect` («первый рабочий выигрывает», паттерн Redis `_anetTcpGenericConnect`: две точки отказа socket/connect + close перед continue, одна успеха break, `p==NULL` после цикла = провал); **lifetime: нельзя трогать `p` (даже `p==NULL`) после `freeaddrinfo`** — dangling-указатель в освобождённый список (прямая параллель с Rust ownership); banner grab как L7 payload поверх готового TCP (ядро уже сняло L2/L3/L4 — связь с B-3); печать IP рабочего узла кастом по `ai_family` + `inet_ntop`+`ntohs` (повтор B-4). Буферизация stdout/stderr (всплыло в тесте): `connected:`(stdout fully-buffered под pipe) печатается после `timeout`(stderr unbuffered) — артефакт перенаправления, см. [[stdio-buffering]]. Конспекты: [[tcp-sockets]] (дополнен — read timeout + EAGAIN/EINTR + клиентский перебор адресов), [[stdio-buffering]] (новый). Код: `networking/host_probe.c`. B-4 — getaddrinfo + DNS resolution: резолв «имя + service → список адресов», две независимые работы внутри (node→DNS/hosts, service→`/etc/services` или число, БЕЗ DNS), комбинируются проставлением порта в каждый адрес; `hints` через `memset` + `AF_UNSPEC` + `SOCK_STREAM` (socktype = фильтр по протокольной колонке `/etc/services`, иначе дубликаты); `addrinfo` как узел связного списка (метаданные `ai_family/socktype/protocol` → в `socket()`, `ai_addr`+`ai_addrlen` → в `connect/bind`, `ai_next`); **извлечение адреса из generic `struct sockaddr *` кастом по `ai_family` (type tag)** → `sockaddr_in`/`sockaddr_in6`, поля `sin_*`/`sin6_*`, `inet_ntop` + `ntohs`; каст не двигает байты (выбирает схему чтения), `getaddrinfo` аллоцирует exact-fit (vs `sockaddr_storage` worst-case в accept/recvfrom); `gai_strerror` (свой namespace, не `errno`); `freeaddrinfo`. Грабли: `%.20s` (precision, режет) vs `%-46s` (width, дополняет); copy-paste IPv6-ветки → NULL deref → SIGSEGV, диагностика через варнинг `set but not used`. Конспекты: [[getaddrinfo]] (дополнен), [[posix-naming]] (новый глоссарий имён). B-2 — TCP-сокеты (socket/bind/listen/accept/connect/send/recv, sockaddr_in + htons/htonl, SO_REUSEADDR/TIME_WAIT, accept value-result socklen_t, three-way recv (0/-1/>0), partial read on stream, EINTR на разных слоях, EPIPE/ECONNRESET как штатное событие, SIGPIPE через sigaction(SIG_IGN), inet_pton/inet_ntop). B-3 — libpcap + парсинг пакетов на проводе: pcap lifecycle (open_live → compile/setfilter → freecode → loop → breakloop → stats → close), BPF фильтр в ядре, callback signature (user + pkthdr + bytes), caplen vs len vs ip_len (три разных длины), layered проверки caplen перед каждым cast'ом, переменная длина IP/TCP заголовков через `ip_hl*4` / `th_off*4` (4-битное поле в 4-байтных словах = self-describing формат), Ethernet/IPv4/TCP layout байт за байтом, network byte order (ntohs обязателен на ether_type/ip_len/порта), pointer arithmetic через cast в `u_char *` (иначе `+N` уезжает на `N*sizeof`), th_flags как битовая маска (AND, не ==), payload_len = `ntohs(ip_len) - ip_hl*4 - th_off*4` (а не из caplen из-за Ethernet padding'а), SA_RESTART=0 для кооперативного выхода через pcap_breakloop, async-signal-safety только у pcap_breakloop, errbuf vs pcap_geterr (handle ещё нет vs есть). B-1 — OWASP Top 10 web (2021) теория: authn vs authz, prepared statement как структурная защита (prepare/execute, данные вне грамматики SQL), XSS reflected/stored/DOM-based, SSRF + cloud metadata 169.254.169.254 (Capital One 2019), allowlist vs denylist + DNS rebinding, IDOR (ID из клиента vs сессии), password hashing (bcrypt/argon2id, не SHA-256), supply chain (xz-utils, event-stream, SolarWinds), insecure deserialization. Принципы: defense in depth, структурные защиты > текстовые, не доверять клиенту. Практика DVWA/PortSwigger — отложена до отдельной сессии.

---

## Блок C — Rust (параллельный трек) — активный (детальный дамп)

**Пройденные темы:** C1.1 — Rust toolchain (rustup/rustc/cargo, Cargo.toml/lock, edition, crate binary vs library, crates.io); главы 1–2 The Book (hello_world, guessing_game). C1.2 — главы 3–4 The Book: типы (`i32`/`u32`/`usize`/`bool`/`char` Unicode, integer overflow панично в debug, wrapping в release), `let`/`mut`/`const`, shadowing (новая переменная того же имени, в т.ч. с другим типом), statements vs expressions (блок и `if` — выражения, `;` отбрасывает значение), control flow (`if` строго `bool`, `loop`/`while`/`for in collection`, ranges `a..b` exclusive / `a..=b` inclusive). Ownership (move vs Copy — `String` not Copy, `i32` is Copy), borrow (`&T` shared N штук / `&mut T` exclusive 1 штука — XOR), `String` (owned, heap) vs `&str` (slice, view), slices `&s[a..b]` без аллокации, deref coercion `&String` → `&str`, dangling reference как compile error (lifetime). Видел реальные сообщения borrow checker'а: E0499 (two &mut), E0502 (mut+immut), E0382 (use after move), E0106/E0515 (dangling) — см. [[ownership]]. C1.3 часть 1 — глава 5 The Book: struct (named-field / tuple / unit-like), field init shorthand, struct update syntax `..base` (move не-Copy полей), `impl` блок, три формы receiver (`&self` / `&mut self` / `self`), associated function vs method (конструктор `new` — конвенциональная associated fn, не языковая конструкция; вызов через `::`), `Self` (большая) как алиас типа, automatic referencing (компилятор сам добавит `&`/`&mut`/`*` к receiver — но **не к аргументам**), `#[derive(Debug)]` + `{:?}` / `{:#?}` / `dbg!(&x)` (stderr + позиция в файле). См. [[structs-methods]]. C1.3 часть 2 — глава 6 The Book: enum с данными разной формы (tuple-like / unit-like / struct-like), `match` как **выражение** (scrutinee → pattern → expression-результат; значение arm'а = значение match'а; уходит через tail expression), exhaustive без `_` (compile error на забытый вариант), деструктуризация + binding в образцах, `println!` (→`()`) vs `format!` (→`String`), `String::from` vs `&str` литерал в arm'ах, `Option<T>` (`Some`/`None`) вместо null, `if let` как сахар над match для одной ветки (теряет exhaustiveness), **match ergonomics** (матч `&T` → биндинг `&T`), slice `&[T]` как borrowed view, **Drop/RAII** (освобождение по владельцу; `for x in vec` move vs `for x in &vec` borrow). Написал `scan_report` (PortState enum + describe + find_port). См. [[enums-match]]. Слабое место: match как выражение зашёл тяжело — нужна практика; владение иногда забывается. C-5 — главы 7–8 The Book: **модульная система** (package ⊃ crate ⊃ module ⊃ item; `main.rs` сам root module = `crate`; модуль объявляется через `mod`, не «по отсутствию main»), пути `crate::`/`super::`/`self::` (FS-аналог `/`÷`../`÷`./`), видимость (`pub`; private по умолчанию, предок не видит приватное потомка; `E0603 is private` — ошибка видимости, не «путь не найден»; асимметрия `pub enum` авто-открывает варианты vs `pub struct` НЕ открывает поля), `use` как ярлык имени (не `#include`, кода не копирует; без `use` — полный путь), prelude (`Vec`/`String`/`Option` из коробки, `HashMap` — нет). **Collections:** `Vec` (push, итерация borrow vs move, `v[i]` runtime panic vs `v.get`→`Option`), `String` (push_str/format!, `&format!` через deref coercion), `HashMap` (ключу `Eq+Hash`, **`entry` API** `or_insert_with` ленивый / `or_insert` eager, `*map.entry(k).or_insert(0)+=1` — разыменование `&mut`, `host.clone()` т.к. `String` не вынести из `&`). Написал `scan_aggregator` (3 модуля). См. [[modules]], [[collections]]. Слабые места (Interview 02): ownership-терминология (тянет к «дропнулось» вместо «отдал владение/move»), уровни package/crate/module.

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
