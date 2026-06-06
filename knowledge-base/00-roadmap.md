---
обновлено: 2026-06-06 (B-5 выполнено — интеграционное host-probe.c: resolve → connect по списку → banner grab; SO_RCVTIMEO + EAGAIN vs EINTR; конспект tcp-sockets дополнен + создан stdio-buffering)
---

# Roadmap — хаб проекта

Здесь — **текущая позиция, журнал выполненного, индекс тем**. Самый часто обновляемый файл.
План обучения и принципы: [strategy/learning-strategy.md](../.claude/strategy/learning-strategy.md).
Контекст о пользователе и реестр скиллов: [.claude/CLAUDE.md](../.claude/CLAUDE.md).

Код: `linux/`, `networking/`, и т.д. | Теория: `topics/` | Задания: `tasks/`

---

## Текущая позиция

> Обновляется после каждой завершённой сессии через `session-debrief`. **Это primary source** для всех skills (give-task, mock-interview, reading-code, ctf).

**Активный блок:** B — Сетевой стек (C).

**Структура блока:** **26 заданий** (20 атомарных + 5 интеграционных + 1 магнум опус). Интеграционные на позициях 5/10/15/20/25, магнум опус — 26. Карта тем зафиксирована 2026-05-25, см. [strategy/learning-strategy.md](../.claude/strategy/learning-strategy.md) → "### Блок B". Базовая структура блока (21) растягивается под глубину темы — см. "## Структура блока" в стратегии.

**Магнум опус (B-26):** network scanner — ARP discovery + ICMP ping sweep + TCP connect scan + TCP SYN scan + service detection. Отдельная репа на GitHub.

**Позиция в блоке B:** 5/26 выполнено (B-1 OWASP web теория, B-2 TCP echo, B-3 pcap sniffer, B-4 getaddrinfo, B-5 host-probe интеграционное). **Следующее — атомарное B-6** (Non-blocking I/O: `fcntl(O_NONBLOCK)`, `EAGAIN`/`EWOULDBLOCK`, `EINPROGRESS` на connect, `SO_RCVTIMEO`/`SO_SNDTIMEO`). Фаза 2 блока.

**Параллельно:** блок C — Rust, C-5 завершён (главы 7–8 The Book — modules + collections, task-c05 `scan_aggregator`). Следующая Rust-сессия — **C-6: глава 9** (error handling — `Result`/`?`, закрывает вопрос 4 из [[guessing-game-notes]]). Карта блока C по главам — в [strategy/learning-strategy.md](../.claude/strategy/learning-strategy.md) → "### Блок C".

**Блок A:** закрыт задним числом по старой схеме (8 атомарных + Interview 01 reinforce). Темы добираются в блоке G по новой структуре.

**Фундамент (theory 2026-05-14):** syscalls + kernel/user boundary, fd kernel model + refcount, async-signal-safety, virtual memory + copy-on-write, EINTR семантика.

**Выполненные темы блока A:** /proc filesystem, directory traversal, file descriptors + readlink, snprintf/qsort, error handling, fork/exec/wait, signals (sigaction/SIGINT/SIGTERM/pause), pipes/IPC (pipe/dup2/read/write), /proc/net/tcp parsing (sscanf, bitwise ops, little-endian IP), pthreads (pthread_create/join, mutex, data race), interview-01 reinforce (opendir/readdir errors, getppid vs fork return, WIFEXITED/WEXITSTATUS, async-signal-safety, volatile sig_atomic_t, SA_RESTART, pipe write-end counter, char* vs char[]).

**Выполненные темы блока B:** B-5 (интеграционное) — `host-probe` (banner grab): синтез B-2/B-3/B-4. **Read timeout `SO_RCVTIMEO`** (иначе recv на молчащем сервере висит вечно — клинч HTTP :80, где сервер ждёт запрос); **различение двух `-1` на recv: `EAGAIN`/`EWOULDBLOCK`(таймаут истёк → принять как «баннера нет») vs `EINTR`(сигнал → retry)** — противоположные реакции, идиома «съесть EINTR в `do/while` до разбора веток»; перебор адресов `getaddrinfo` с попыткой `connect` («первый рабочий выигрывает», паттерн Redis `_anetTcpGenericConnect`: две точки отказа socket/connect + close перед continue, одна успеха break, `p==NULL` после цикла = провал); **lifetime: нельзя трогать `p` (даже `p==NULL`) после `freeaddrinfo`** — dangling-указатель в освобождённый список (прямая параллель с Rust ownership); banner grab как L7 payload поверх готового TCP (ядро уже сняло L2/L3/L4 — связь с B-3); печать IP рабочего узла кастом по `ai_family` + `inet_ntop`+`ntohs` (повтор B-4). Буферизация stdout/stderr (всплыло в тесте): `connected:`(stdout fully-buffered под pipe) печатается после `timeout`(stderr unbuffered) — артефакт перенаправления, см. [[stdio-buffering]]. Конспекты: [[tcp-sockets]] (дополнен — read timeout + EAGAIN/EINTR + клиентский перебор адресов), [[stdio-buffering]] (новый). Код: `networking/host_probe.c`. B-4 — getaddrinfo + DNS resolution: резолв «имя + service → список адресов», две независимые работы внутри (node→DNS/hosts, service→`/etc/services` или число, БЕЗ DNS), комбинируются проставлением порта в каждый адрес; `hints` через `memset` + `AF_UNSPEC` + `SOCK_STREAM` (socktype = фильтр по протокольной колонке `/etc/services`, иначе дубликаты); `addrinfo` как узел связного списка (метаданные `ai_family/socktype/protocol` → в `socket()`, `ai_addr`+`ai_addrlen` → в `connect/bind`, `ai_next`); **извлечение адреса из generic `struct sockaddr *` кастом по `ai_family` (type tag)** → `sockaddr_in`/`sockaddr_in6`, поля `sin_*`/`sin6_*`, `inet_ntop` + `ntohs`; каст не двигает байты (выбирает схему чтения), `getaddrinfo` аллоцирует exact-fit (vs `sockaddr_storage` worst-case в accept/recvfrom); `gai_strerror` (свой namespace, не `errno`); `freeaddrinfo`. Грабли: `%.20s` (precision, режет) vs `%-46s` (width, дополняет); copy-paste IPv6-ветки → NULL deref → SIGSEGV, диагностика через варнинг `set but not used`. Конспекты: [[getaddrinfo]] (дополнен), [[posix-naming]] (новый глоссарий имён). B-2 — TCP-сокеты (socket/bind/listen/accept/connect/send/recv, sockaddr_in + htons/htonl, SO_REUSEADDR/TIME_WAIT, accept value-result socklen_t, three-way recv (0/-1/>0), partial read on stream, EINTR на разных слоях, EPIPE/ECONNRESET как штатное событие, SIGPIPE через sigaction(SIG_IGN), inet_pton/inet_ntop). B-3 — libpcap + парсинг пакетов на проводе: pcap lifecycle (open_live → compile/setfilter → freecode → loop → breakloop → stats → close), BPF фильтр в ядре, callback signature (user + pkthdr + bytes), caplen vs len vs ip_len (три разных длины), layered проверки caplen перед каждым cast'ом, переменная длина IP/TCP заголовков через `ip_hl*4` / `th_off*4` (4-битное поле в 4-байтных словах = self-describing формат), Ethernet/IPv4/TCP layout байт за байтом, network byte order (ntohs обязателен на ether_type/ip_len/порта), pointer arithmetic через cast в `u_char *` (иначе `+N` уезжает на `N*sizeof`), th_flags как битовая маска (AND, не ==), payload_len = `ntohs(ip_len) - ip_hl*4 - th_off*4` (а не из caplen из-за Ethernet padding'а), SA_RESTART=0 для кооперативного выхода через pcap_breakloop, async-signal-safety только у pcap_breakloop, errbuf vs pcap_geterr (handle ещё нет vs есть). B-1 — OWASP Top 10 web (2021) теория: authn vs authz, prepared statement как структурная защита (prepare/execute, данные вне грамматики SQL), XSS reflected/stored/DOM-based, SSRF + cloud metadata 169.254.169.254 (Capital One 2019), allowlist vs denylist + DNS rebinding, IDOR (ID из клиента vs сессии), password hashing (bcrypt/argon2id, не SHA-256), supply chain (xz-utils, event-stream, SolarWinds), insecure deserialization. Принципы: defense in depth, структурные защиты > текстовые, не доверять клиенту. Практика DVWA/PortSwigger — отложена до отдельной сессии.

**Выполненные темы блока C:** C1.1 — Rust toolchain (rustup/rustc/cargo, Cargo.toml/lock, edition, crate binary vs library, crates.io); главы 1–2 The Book (hello_world, guessing_game). C1.2 — главы 3–4 The Book: типы (`i32`/`u32`/`usize`/`bool`/`char` Unicode, integer overflow панично в debug, wrapping в release), `let`/`mut`/`const`, shadowing (новая переменная того же имени, в т.ч. с другим типом), statements vs expressions (блок и `if` — выражения, `;` отбрасывает значение), control flow (`if` строго `bool`, `loop`/`while`/`for in collection`, ranges `a..b` exclusive / `a..=b` inclusive). Ownership (move vs Copy — `String` not Copy, `i32` is Copy), borrow (`&T` shared N штук / `&mut T` exclusive 1 штука — XOR), `String` (owned, heap) vs `&str` (slice, view), slices `&s[a..b]` без аллокации, deref coercion `&String` → `&str`, dangling reference как compile error (lifetime). Видел реальные сообщения borrow checker'а: E0499 (two &mut), E0502 (mut+immut), E0382 (use after move), E0106/E0515 (dangling) — см. [[ownership]]. C1.3 часть 1 — глава 5 The Book: struct (named-field / tuple / unit-like), field init shorthand, struct update syntax `..base` (move не-Copy полей), `impl` блок, три формы receiver (`&self` / `&mut self` / `self`), associated function vs method (конструктор `new` — конвенциональная associated fn, не языковая конструкция; вызов через `::`), `Self` (большая) как алиас типа, automatic referencing (компилятор сам добавит `&`/`&mut`/`*` к receiver — но **не к аргументам**), `#[derive(Debug)]` + `{:?}` / `{:#?}` / `dbg!(&x)` (stderr + позиция в файле). См. [[structs-methods]]. C1.3 часть 2 — глава 6 The Book: enum с данными разной формы (tuple-like / unit-like / struct-like), `match` как **выражение** (scrutinee → pattern → expression-результат; значение arm'а = значение match'а; уходит через tail expression), exhaustive без `_` (compile error на забытый вариант), деструктуризация + binding в образцах, `println!` (→`()`) vs `format!` (→`String`), `String::from` vs `&str` литерал в arm'ах, `Option<T>` (`Some`/`None`) вместо null, `if let` как сахар над match для одной ветки (теряет exhaustiveness), **match ergonomics** (матч `&T` → биндинг `&T`), slice `&[T]` как borrowed view, **Drop/RAII** (освобождение по владельцу; `for x in vec` move vs `for x in &vec` borrow). Написал `scan_report` (PortState enum + describe + find_port). См. [[enums-match]]. Слабое место: match как выражение зашёл тяжело — нужна практика; владение иногда забывается. C-5 — главы 7–8 The Book: **модульная система** (package ⊃ crate ⊃ module ⊃ item; `main.rs` сам root module = `crate`; модуль объявляется через `mod`, не «по отсутствию main»), пути `crate::`/`super::`/`self::` (FS-аналог `/`÷`../`÷`./`), видимость (`pub`; private по умолчанию, предок не видит приватное потомка; `E0603 is private` — ошибка видимости, не «путь не найден»; асимметрия `pub enum` авто-открывает варианты vs `pub struct` НЕ открывает поля), `use` как ярлык имени (не `#include`, кода не копирует; без `use` — полный путь), prelude (`Vec`/`String`/`Option` из коробки, `HashMap` — нет). **Collections:** `Vec` (push, итерация borrow vs move, `v[i]` runtime panic vs `v.get`→`Option`), `String` (push_str/format!, `&format!` через deref coercion), `HashMap` (ключу `Eq+Hash`, **`entry` API** `or_insert_with` ленивый / `or_insert` eager, `*map.entry(k).or_insert(0)+=1` — разыменование `&mut`, `host.clone()` т.к. `String` не вынести из `&`). Написал `scan_aggregator` (3 модуля). См. [[modules]], [[collections]]. Слабые места (Interview 02): ownership-терминология (тянет к «дропнулось» вместо «отдал владение/move»), уровни package/crate/module.

**Последний выполненный task:** task-b05 (B-5) — интеграционное `host-probe` banner grab (`networking/host_probe.c`), 2026-06-06.

**Следующий шаг:** **B-6 — атомарное Non-blocking I/O.** `fcntl(O_NONBLOCK)`, `EAGAIN`/`EWOULDBLOCK` на чтении без данных, `EINPROGRESS` на non-blocking connect, `SO_RCVTIMEO`/`SO_SNDTIMEO` как альтернатива. Фаза 2 блока B (non-blocking + connect-scan). Код: `networking/`. Детали — стратегия → "### Блок B".

Параллельный трек (Rust) **намеренно ведётся линейно по главам The Book** — одна глава = одно (обычно атомарное) задание `C-N`. Под структуру «4 атомарных + интеграционное + опус» он не переводится: правило «каждое 5-е интеграционное» к Rust-треку не применяется. См. [strategy/learning-strategy.md](../.claude/strategy/learning-strategy.md) → "## Структура блока" → исключение для учебник-driven блоков.

**Параллельные блоки/треки доступны сейчас:**
- **Блок C — Rust**. Главы 7–8 завершены (modules + collections): [[modules]], [[collections]], task-c05 `scan_aggregator`. Следующий шаг: **C-6 — глава 9** (error handling — `Result`/`?`, закрывает вопрос 4 из [[guessing-game-notes]]). Дальше по карте: гл.10 (traits/generics/lifetimes), гл.11 (тесты), гл.12 (minigrep). Замечания: ownership-терминология (move vs «дроп») и уровни package/crate/module требуют закрепления (Interview 02).
- **B-1 — OWASP Top 10 (web)** — теория завершена (task-b01, 2026-05-20). Конспект: [[owasp-top10]]. Практика DVWA/PortSwigger labs (3–5 челленджей) отложена.
- **Мок-собес / reading-code / CTF** — управляются скиллами (по запросу пользователя или решению Claude), не счётчиками. См. таблицу триггеров в [.claude/CLAUDE.md](../.claude/CLAUDE.md).

При запросе задания `give-task` обязан учитывать что пользователь может выбрать главный трек **или** параллельный. Если параллельные блоки доступны — спросить какой трек.

**Зафиксированный ритм 3:1 (B : C)** — договорённость 2026-06-04, на время совместного прохождения B и C. На каждые 3 задания из B — 1 из C; `give-task` по умолчанию рекомендует B, переключая на C каждое 4-е. Текущее состояние (C=5, B=5) — перекос почти выправлен, ближайшие задания из B (ориентир ~B=8/26); с последней C-задачи (C-5) прошло 2 B-задачи (B-4, B-5), до C по ритму ещё ~1 B. Пользователь может перебить ритм. Детали — `give-task` skill → шаг 4. Снять ритм при закрытии B-26 или конца карты C.

### Невыполненные закрепляющие задания

Пусто. *(Interview 02 reinforce закрыт вместе с C-5 — пробелы отработаны в `scan_aggregator` + разминке task-c05. Остаточные слабые места ownership-терминология / package-crate-module перенесены в "Выполненные темы блока C" как фокус следующих Rust-сессий.)*

*(После каждого мок-собеса сюда добавляются `[[task-NN-interviewMM-reinforce]]` — закрывать до перехода к следующей теме плана.)*

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

### Блок B — Сетевой стек (C) — активный (5/26)

| # | Задание | Статус | Код |
|---|---------|--------|-----|
| B-1 | [[task-b01-owasp-top10\|OWASP Top 10 (web) теория]] | ✅ выполнено (2026-05-20) | — (reading + vault) |
| B-2 | [[task-b02-tcp-echo\|TCP echo server + client]] | ✅ выполнено (2026-05-17) | `networking/echo_server.c`, `networking/echo_client.c` |
| B-3 | [[task-b03-pcap-sniffer\|pcap sniffer — Ethernet → IP → TCP]] | ✅ выполнено (2026-05-25) | `networking/sniffer.c` |
| B-4 | [[task-b04-getaddrinfo\|getaddrinfo + DNS resolution (resolve)]] | ✅ выполнено (2026-06-05) | `networking/resolve.c` |
| B-5 | [[task-b05-host-probe\|host-probe — banner grab (integration)]] | ✅ выполнено (2026-06-06) | `networking/host_probe.c` |

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

Детальные записи: `knowledge-base/interviews/interview-NN.md`.
Закрепляющие задания после каждого мок-собеса — оформляются как `task-<блок><NN>-interviewMM-reinforce.md` (нумерация внутри блока) и попадают в раздел "Невыполненные закрепляющие задания" (Текущая позиция). После выполнения — в "Задачи — writing" под своим блоком.

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

---

## Темы — Linux

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

## Темы — Rust

- [[toolchain]] — rustup, rustc, cargo, Cargo.toml/lock, crate, edition, crates.io
- [[guessing-game-notes]] — открытые вопросы после главы 2 The Book (закрыто 1,2,3,5,6,7; открыто 4,8)
- [[types-control-flow]] — глава 3: immutable by default, shadowing, типы, statements vs expressions, ranges
- [[ownership]] — глава 4: move/copy, `&T`/`&mut T`, slices, dangling, реальные сообщения borrow checker'а, Drop/RAII (освобождение по владельцу)
- [[structs-methods]] — глава 5: struct (named/tuple/unit), `impl`, `&self`/`&mut self`/`self`, associated function vs method, `Self`, automatic referencing, `#[derive(Debug)]` + `{:?}`/`{:#?}`/`dbg!`
- [[enums-match]] — глава 6: enum с данными, `match` как выражение (exhaustive), `Option<T>` вместо null, `if let`, match ergonomics
- [[modules]] — глава 7: package ⊃ crate ⊃ module ⊃ item, пути `crate`/`super`/`self`, `pub`-видимость + `E0603`, `use` как ярлык (не `#include`), prelude
- [[collections]] — глава 8: `Vec`/`String`/`HashMap`, `entry` API (`or_insert_with`/`or_insert`), `*`-разыменование счётчика, `v[i]` vs `v.get`, итерация borrow vs move

## Темы — Networking

- [[tcp-sockets]] — TCP-сокеты, socket/bind/listen/accept/connect, partial read, EPIPE, SIGPIPE защита
- [[event-loop-epoll]] — event loop model, epoll, non-blocking I/O, readiness ≠ correctness
- [[nagle-tcp-nodelay]] — алгоритм Нагла, TCP_NODELAY, когда выключать
- [[getaddrinfo]] — resolve, addrinfo linked list, gai_strerror, паттерн "свой namespace ошибок"
- [[libpcap]] — захват пакетов из user space, BPF фильтр в ядре, pcap lifecycle, caplen vs len, breakloop
- [[ethernet-frame]] — layout Ethernet/IPv4/TCP в памяти, ip_hl*4 / th_off*4, NBO, pointer arithmetic через u_char *, payload length

## Темы — Security / CTF

- [[command-injection]] — command injection в picoCTF ping-cmd, защита через `execve`
- [[owasp-top10]] — OWASP Top 10 (web, 2021): 10 категорий мышления, prepared statement / XSS / SSRF / IDOR глубоко
- [[session-02-writeup]] — Forensics session-02: 4 таска (метаданные, sha256sum+openssl, magic bytes JPEG, polyglot PNG+PDF); таблица инструментов forensics (`exiftool`, `xxd`, `dd conv=notrunc`, `grep -aob`)

## Темы — C

- [[file-io-c]] — fopen/fgets/fclose vs open/read
- [[directory-traversal-c]] — opendir/readdir/closedir
- [[string-formatting-c]] — snprintf, sscanf, strncmp
- [[error-handling-c]] — обработка ошибок, errno, perror
- [[sorting-c]] — qsort, компаратор
- [[bitwise-flags]] — битовые флаги, идиомы (`|=`, `&= ~`, `!!`), file status vs descriptor flags
- [[posix-naming]] — словарь сокращений в именах типов/функций (`in`/`sa`/`ai`/`sin6`, зоопарк sockaddr*, width vs precision, byte order `n`/`h`)
- [[stdio-buffering]] — буферизация stdout (line/full/unbuffered) vs stderr, переплетение строк под pipe, fflush, потеря вывода при краше

## Темы — Reading (разборы чужого кода)

- [[musl-popen]] — musl libc popen/pclose/_Fork, fd inheritance, FD_CLOEXEC
- [[redis-anet]] — Redis обёртка над sockets API: симметрия connect/server, гигиена сокета, паттерн goto error

---

## Этапы

| Этап | Якорный проект | Когда | Статус |
|------|---------------|-------|--------|
| 1. Фундамент | Network scanner (C) + prompt injection detector (Python) | сейчас — конец 2026 | в работе (блок B) |
| 2. VPN | Упрощённый VPN-клиент (Rust) | 2027 | впереди |
| 3. Изоляция и наблюдение | LLM-jail: sandbox для LLM-агентов с eBPF-телеметрией | 2027–2028 | впереди |

---

## Чекпойнты этапа 1 (конец 2026)

- [ ] Network scanner работает: находит хосты, сканирует порты, определяет сервисы (отдельная репа на GitHub с README)
- [ ] Prompt injection detector работает (отдельная репа на GitHub с README, тестами, лицензией)
- [ ] Понимаешь что происходит при `connect()` на уровне ядра
- [ ] Можешь написать простую структуру данных на Rust без подсказок
- [ ] OWASP Top 10 (web и LLM) — могу объяснить на пальцах
- [ ] picoCTF: минимум 30 решённых челленджей (сейчас: 5)
- [ ] Reading-code: ≥ 6 разборов чужого кода в `topics/reading/` (сейчас: 2)
