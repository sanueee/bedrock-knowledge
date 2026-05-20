---
обновлено: 2026-05-20 (task-13)
---

# Roadmap — хаб проекта

Здесь — **текущая позиция, журнал выполненного, индекс тем**. Самый часто обновляемый файл.
План обучения и принципы: [strategy/learning-strategy.md](../.claude/strategy/learning-strategy.md).
Контекст о пользователе и реестр скиллов: [.claude/CLAUDE.md](../.claude/CLAUDE.md).

Код: `linux/`, `networking/`, и т.д. | Теория: `topics/` | Задания: `tasks/`

---

## Текущая позиция

> Обновляется после каждой завершённой сессии через `session-debrief`. **Это primary source** для всех skills (give-task, mock-interview, reading-code, ctf).

**Активный блок:** B — Сетевой стек (C). B1 завершён, B0 (OWASP теория) завершён. Параллельно: блок C — Rust, C1.2 завершён.

**Фундамент (theory 2026-05-14):** syscalls + kernel/user boundary, fd kernel model + refcount, async-signal-safety, virtual memory + copy-on-write, EINTR семантика.

**Выполненные темы блока A:** /proc filesystem, directory traversal, file descriptors + readlink, snprintf/qsort, error handling, fork/exec/wait, signals (sigaction/SIGINT/SIGTERM/pause), pipes/IPC (pipe/dup2/read/write), /proc/net/tcp parsing (sscanf, bitwise ops, little-endian IP), pthreads (pthread_create/join, mutex, data race), interview-01 reinforce (opendir/readdir errors, getppid vs fork return, WIFEXITED/WEXITSTATUS, async-signal-safety, volatile sig_atomic_t, SA_RESTART, pipe write-end counter, char* vs char[]).

**Выполненные темы блока B:** B1 — TCP-сокеты (socket/bind/listen/accept/connect/send/recv, sockaddr_in + htons/htonl, SO_REUSEADDR/TIME_WAIT, accept value-result socklen_t, three-way recv (0/-1/>0), partial read on stream, EINTR на разных слоях, EPIPE/ECONNRESET как штатное событие, SIGPIPE через sigaction(SIG_IGN), inet_pton/inet_ntop). B0 — OWASP Top 10 web (2021) теория: authn vs authz, prepared statement как структурная защита (prepare/execute, данные вне грамматики SQL), XSS reflected/stored/DOM-based, SSRF + cloud metadata 169.254.169.254 (Capital One 2019), allowlist vs denylist + DNS rebinding, IDOR (ID из клиента vs сессии), password hashing (bcrypt/argon2id, не SHA-256), supply chain (xz-utils, event-stream, SolarWinds), insecure deserialization. Принципы: defense in depth, структурные защиты > текстовые, не доверять клиенту. Практика DVWA/PortSwigger — отложена до отдельной сессии.

**Выполненные темы блока C:** C1.1 — Rust toolchain (rustup/rustc/cargo, Cargo.toml/lock, edition, crate binary vs library, crates.io); главы 1–2 The Book (hello_world, guessing_game). C1.2 — главы 3–4 The Book: типы (`i32`/`u32`/`usize`/`bool`/`char` Unicode, integer overflow панично в debug, wrapping в release), `let`/`mut`/`const`, shadowing (новая переменная того же имени, в т.ч. с другим типом), statements vs expressions (блок и `if` — выражения, `;` отбрасывает значение), control flow (`if` строго `bool`, `loop`/`while`/`for in collection`, ranges `a..b` exclusive / `a..=b` inclusive). Ownership (move vs Copy — `String` not Copy, `i32` is Copy), borrow (`&T` shared N штук / `&mut T` exclusive 1 штука — XOR), `String` (owned, heap) vs `&str` (slice, view), slices `&s[a..b]` без аллокации, deref coercion `&String` → `&str`, dangling reference как compile error (lifetime). Видел реальные сообщения borrow checker'а: E0499 (two &mut), E0502 (mut+immut), E0382 (use after move), E0106/E0515 (dangling) — см. [[ownership]].

**Последний выполненный task:** task-13 — OWASP Top 10 web теория (2026-05-20).

**Следующий шаг — B2: разбор пакетов (libpcap)**
- Перехват и парсинг пакетов на уровне Ethernet/IP/TCP.
- Библиотека: libpcap (нужна установка `libpcap-dev`).
- Код: `networking/sniffer/` или подобное.
- Vault: `knowledge-base/tasks/task-11-pcap.md`, темы: `topics/networking/libpcap.md`, `topics/networking/ethernet-frame.md`.
- Перед началом перечитать: `topics/networking/tcp-sockets.md` (host vs network byte order), `topics/linux/proc-net.md` (структура IP/TCP заголовков на уровне байтов).

**Параллельные блоки/треки доступны сейчас:**
- **Блок C — Rust**. C1.2 (The Book гл. 3–4) завершено — Ownership, borrow, slices, типы, control flow. Следующий шаг: **C1.3 — главы 5–6** (structs, enums, match exhaustive — закрывает вопрос 6 из [[guessing-game-notes]]). Чередовать с B-задачами сессиями.
- **B0 — OWASP Top 10 (web)** — **теория завершена (task-13, 2026-05-20)**. Конспект: [[owasp-top10]]. Практика DVWA/PortSwigger labs (3–5 челленджей) отложена — добавить отдельной сессией перед B5 TLS.
- **picoCTF** — управляется счётчиком CTF.
- **Reading-code** — управляется счётчиком reading.

При запросе задания `give-task` обязан учитывать что пользователь может выбрать главный трек (B2) **или** параллельный (C1 / B0 / CTF). Если параллельные блоки доступны — спросить какой трек, не выдавать B2 по умолчанию молча.

### Счётчики

- **Счётчик задач с последнего мок-собеса:** 6 *(каждые 8 — предложить `mock-interview`)*
- **Счётчик задач с последнего reading-code:** 4 *(каждые 4 — обязательно `reading-code`. Также обязательно при завершении любого блока. Reading-01 — 2026-05-08.)* **→ триггер сработал, следующая сессия должна быть reading-code.**
- **Счётчик задач с последнего CTF:** 5 *(каждые 5 — мягко предложить `ctf`. Параллельный трек, ~1-2 часа на picoCTF.)* **→ триггер сработал, после reading можно предложить CTF.**

### Невыполненные закрепляющие задания

Пусто. *(После каждого мок-собеса сюда добавляются `[[task-NN-interviewMM-reinforce]]` — закрывать до перехода к следующей теме плана.)*

---

## Задачи — writing

| # | Задание | Статус | Код |
|---|---------|--------|-----|
| 01 | [[task-01-procinfo\|procinfo]] | ✅ выполнено | `linux/proc/procinfo.c` |
| 02 | [[task-02-procinfo-top5\|procinfo top5]] | ✅ выполнено | `linux/proc/procinfo2.c` |
| 03 | [[task-03-fdlist\|fdlist]] | ✅ выполнено | `linux/fd/fdlist.c` |
| 04 | [[task-04-procfork\|procfork]] | ✅ выполнено | `linux/proc/procfork.c` |
| 05 | [[task-05-signals\|signals]] | ✅ выполнено | `linux/proc/signals.c` |
| 06 | [[task-06-pipes\|pipes]] | ✅ выполнено | `linux/proc/pipechat.c` |
| 07 | [[task-07-procnet\|procnet]] | ✅ выполнено | `linux/proc/procnet.c` |
| 08 | [[task-08-pthreads\|pthreads]] | ✅ выполнено | `linux/proc/procthreads.c` |
| 09 | [[task-09-interview01-reinforce\|interview-01 reinforce]] | ✅ выполнено | `linux/reinforce/` |
| 10 | [[task-10-tcp-echo\|TCP echo server + client]] | ✅ выполнено (2026-05-17) | `networking/echo_server.c`, `networking/echo_client.c` |
| 11 | [[task-11-rust-ch1-2\|Rust: toolchain + hello_world + Guessing Game]] | ✅ выполнено (2026-05-18) | `rust/hello_world/`, `rust/guessing_game/` |
| 12 | [[task-12-rust-ch3-4\|Rust ch.3–4 + word_tools (slices/ownership)]] | ✅ выполнено (2026-05-18) | `rust/word_tools/src/main.rs` |
| 13 | [[task-13-owasp-top10\|OWASP Top 10 (web) теория]] | ✅ выполнено (2026-05-20) | — (reading + vault) |

---

## Reading-сессии

| # | Дата | Источник | Цель | Vault |
|---|------|----------|------|-------|
| 01 | 2026-05-08 | musl libc | popen/pclose/_Fork — fd inheritance, FD_CLOEXEC | [[musl-popen]] |

---

## Theory-сессии

| # | Дата | Темы | Vault |
|---|------|------|-------|
| 1 | 2026-05-14 | фундамент перед B1: syscalls, kernel/user boundary, fd model + refcount, async-signal-safety, copy-on-write, EINTR | [[syscalls-linux]], [[fd-kernel-model]], [[async-signal-safe]], [[virtual-memory-cow]] |

---

## Мок-собесы

| # | Дата | Слабые места |
|---|------|-------------|
| 01 | 2026-04-28 | opendir vs readdir, зомби/таблица процессов, sigaction vs signal, pipe EOF |

Детальные записи: `knowledge-base/interviews/interview-NN.md`.
Закрепляющие задания после каждого мок-собеса — оформляются как `task-NN-interviewMM-reinforce.md` и попадают в раздел "Невыполненные закрепляющие задания" (Текущая позиция). После выполнения — в "Задачи — writing".

---

## CTF

| # | Дата | Платформа | Категория | Таск | Главное |
|---|------|-----------|-----------|------|---------|
| 01 | 2026-04-30 | picoCTF | General Skills | [[ctf/picoctf/general-skills/ping-cmd\|ping-cmd]] | command injection через `&&`; space vs shell metacharacter; `execve` over `system` |

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
- [[guessing-game-notes]] — открытые вопросы после главы 2 The Book (закрыто 1,2,3,5,7; открыто 4,6,8)
- [[types-control-flow]] — глава 3: immutable by default, shadowing, типы, statements vs expressions, ranges
- [[ownership]] — глава 4: move/copy, `&T`/`&mut T`, slices, dangling, реальные сообщения borrow checker'а

## Темы — Networking

- [[tcp-sockets]] — TCP-сокеты, socket/bind/listen/accept/connect, partial read, EPIPE, SIGPIPE защита

## Темы — Security / CTF

- [[command-injection]] — command injection в picoCTF ping-cmd, защита через `execve`
- [[owasp-top10]] — OWASP Top 10 (web, 2021): 10 категорий мышления, prepared statement / XSS / SSRF / IDOR глубоко

## Темы — C

- [[file-io-c]] — fopen/fgets/fclose vs open/read
- [[directory-traversal-c]] — opendir/readdir/closedir
- [[string-formatting-c]] — snprintf, sscanf, strncmp
- [[error-handling-c]] — обработка ошибок, errno, perror
- [[sorting-c]] — qsort, компаратор

## Темы — Reading (разборы чужого кода)

- [[musl-popen]] — musl libc popen/pclose/_Fork, fd inheritance, FD_CLOEXEC

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
- [ ] picoCTF: минимум 30 решённых челленджей
- [ ] Reading-code: ≥ 6 разборов чужого кода в `topics/reading/` (сейчас: 1)
