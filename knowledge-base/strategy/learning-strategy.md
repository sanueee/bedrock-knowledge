# Стратегия обучения — fn2s

Этот файл — **план и принципы** (будущее). Только то, что ещё впереди:
- **Текущий блок** расписан по заданиям (одно задание = краткая сводка). По мере выполнения задания **удаляются** отсюда — выполненное живёт в `roadmap/`, не здесь.
- **Будущие блоки** описаны прозой в пару абзацев. Подробная карта тем проектируется, когда наступает очередь блока, не заранее.

Статус (что выполнено, где сейчас, слабые места) — в диспетчере [knowledge-base/roadmap/00-roadmap.md](../roadmap/00-roadmap.md) → "Текущая позиция". Журналы заданий — `roadmap/journal.md`, пройденные темы — `roadmap/archive.md`, детали мок-собесов — `knowledge-base/interviews/`.

Скиллы (give-task, mock-interview, reading-code, ctf, session-debrief) читают **roadmap/00-roadmap.md** для статуса и **этот файл** для плана/принципов.

---

## Языки в стратегии

**Основные** (под якорные проекты, осознанная глубина):
- **C** — этап 1 (Linux internals, сети, mini-firejail в этапе 3)
- **Rust** — этап 2 (VPN, потом части этапа 3)

**Инструментальные** (учим по необходимости, без отдельных учебных блоков):
- **Python** — везде где LLM-security и SAST: блок Z (prompt injection detector), блоки I и J этапа 3 (LLM-обвязка, NeMo Guardrails, llm-guard, Microsoft Presidio), DevSecOps-tooling (bandit, semgrep), CTF (pwntools для exploit dev). Прокачивается через эти задачи, не отдельным курсом.
- **Go** — для user-space части eBPF-инструментов (libbpf-go, Inspektor Gadget) и для чтения/патчинга чужого кода экосистемы (Cilium, Tetragon, Falco, Trivy). В блоке H можно опционально написать syscall-tracer на Go вместо C — это упростит user-space часть и даст входной билет в Go.

---

## Ближайший план — путь к Positive Technologies

**Ближайшая цель:** попасть на стажировку в Positive Technologies со стеком C/Rust/Linux. PT START — набор дважды в год; целимся в зимний цикл (собес ~январь 2027), чтобы зайти с портфолио, а не «ещё одним студентом». Дифференциация от массы: (1) сильная сессия в универе, (2) реальный Rust, (3) публичные проекты на GitHub.

**Целевое портфолио к январю 2027 (GitHub):**

- `scanner` — C, магнум опус блока B (network scanner).
- `vpn` — Rust, портфолио-MVP (минимальный рабочий туннель).
- `agent-cage` — коммиты в совместный проект с kahooso (eBPF-observability).

**Последовательность:**

1. Блок B (сетевой стек, C) → магнум опус `scanner` - закрыть в июле 2026.
2. Блок C (Rust по The Book) — добить базу языка - закрыть в июле 2026.
3. Блок D (Rust intermediate: tokio/serde/nix) → на его базе портфолио-MVP `vpn` - закрыть до декабря 2026.
4. agent-cage — старт **в июле 2026**.
5. Август–декабрь 2026 — подготовка к PT: довести GitHub до «нестыдного» вида + подготовка к собесу (теория, mock-interview, CTF).
6. Январь 2027 — собес в PT.
7. ~Январь–июль 2027 — стажировка/работа в PT (6 месяцев).

---

## Принципы

1. **Проект раньше языка** — не "учу Rust", а "пишу VPN на Rust". Язык — инструмент, проект — мотивация.
2. **Спираль, не линия** — темы возвращаются на новом уровне. Сначала "как использовать", потом "как работает внутри".
3. **Одна атомарная задача — одна тема** — не смешивать `fork()` и `epoll` в одном атомарном задании.
4. **Каждый task завершается vault-записью** — иначе знание не зафиксировано.
5. **Глубина важнее скорости** — лучше второй раз пройти "то же самое" под другим углом, чем оставить ощущение неполноты. Интеграционные задания и магнум опус — не церемония, а механизм перехода от "видел" к "умею".
6. **Английский — рабочий словарь** — термины и концепции в vault фиксируются на английском (с русским переводом). Будущие мануалы, RFC, собеседования — на английском.

---

## Структура блока

Блок строится по схеме **16 атомарных + 4 интеграционных (каждое 5-е) + 1 магнум опус** (минимум 21, растягивается кратно 5 под глубину темы). Учебник-driven блоки (Rust по The Book) — исключение, идут линейно по главам. Правила структуры, гейт закрытия и процесс проектирования блока (карта тем → размер → опус) — в скилле `plan-block`.

---

## Этап 1 — Фундамент (сейчас — конец 2026)

**Цель этапа:** знать Linux изнутри на уровне системных вызовов, понимать сетевой стек от Ethernet до TLS, написать первый Rust-код.

**Якорный проект: network scanner на C**
Сканер сети: ARP discovery + TCP port scan + определение сервиса по порту.
Не клон nmap — свой инструмент с пониманием каждой строки. Все блоки A и B работают на него.

---

### Блок A — Linux Internals + Data Structures (C) — закрыт

Закрыт задним числом по старой схеме (атомарные без интеграционных и без магнум опуса). Две части:
- **Linux internals.** `/proc`, fork/exec/wait, signals, pipes, pthreads, fd refcount, `/proc/net`. Возвращаются в **блоке G** по новой структуре (21 задание) через интеграционные задания вместе с темами изоляции — это и есть "спираль" из принципа 2.
- **Data structures (экзамен 1 курса).** Полный курс СД на C под экзамен (модули 0–8: типы → линейные → деревья → хеши → графы → сортировки + сквозные темы: мощность/размер, представление в памяти, анализ сложности, проблемы памяти). Не выбрасывается после экзамена — это фундамент под основной вектор. Конспекты — `topics/ds/`, код — `sections/ds/`.

Структура пройденного — в `roadmap/archive.md`.

---

### Блок B — Сетевой стек (C)

> **Структура: 26 заданий** (20 атомарных + 5 интеграционных + 1 магнум опус). Спроектировано 2026-05-25.
> Интеграционные на позициях **5, 10, 15, 20, 25**. Магнум опус — позиция **26**.

**Магнум опус (B-26): network scanner.** Отдельная репа на GitHub.
- ARP discovery (layer 2, `PF_PACKET`) для локальной подсети
- ICMP ping sweep (layer 3, raw socket) для маршрутизируемых сетей
- TCP connect scan (non-blocking + epoll, без root)
- TCP SYN scan (raw socket send + pcap capture, с `CAP_NET_RAW`)
- Service detection: библиотека пробов (SSH banner, HTTP HEAD, SMTP greeting и т.д.)
- CLI: CIDR-диапазоны, список портов, выбор техники, output форматы (text/json)
- README, tests, CI (GitHub Actions с clang-tidy + ASan/UBSan)

Место: `networking/scanner/` (внутри fn2s) + **отдельная репа на GitHub**.

#### Фаза 3 — Application protocols + service detection

**B-14 (atomic) — TLS handshake observational.** Wireshark на HTTPS-сессии, разбор ClientHello/ServerHello/Certificate/Finished, SNI extension, ALPN. **Без кода.** Место: `topics/networking/tls.md`. Закрывает чекпойнт этапа 1 "понимаешь что происходит при connect() на уровне ядра" — расширяет на L7.

**B-15 (integration) — scanner v2.** Scan диапазона + identification сервиса для каждого открытого порта (используя пробы из B-13). Синтез B-11/B-12/B-13 поверх B-10.

#### Фаза 4 — Raw sockets + L2/L3

**B-16 (atomic) — Interface enumeration.** `getifaddrs` для списка интерфейсов с их IP и масками, парсинг `/proc/net/route` для default gateway. Откуда сканер вообще знает в какой он подсети.

**B-17 (atomic) — Raw sockets и capabilities.** `SOCK_RAW`, модель привилегий, `CAP_NET_RAW` через `libcap-ng`, `setcap cap_net_raw+ep` на бинарник вместо запуска под root. Принцип "minimum capability surface".

**B-18 (atomic) — Internet checksum.** RFC 1071, алгоритм one's complement sum, конструкция IP/TCP заголовков в памяти, псевдо-заголовок для TCP-checksum'а.

**B-19 (atomic) — ICMP echo.** Ping одного хоста через raw socket: построить ICMP echo request, отправить, поймать echo reply, измерить RTT.

**B-20 (integration) — ICMP ping sweep.** Sweep по `/24`: параллельная отправка echo requests, асинхронный сбор ответов с тайм-аутом. Синтез B-16 (откуда `/24`) + B-17 (raw + caps) + B-18 (checksum) + B-19 (один ICMP).

#### Фаза 5 — ARP + SYN scan + threading

**B-21 (atomic) — ARP protocol + PF_PACKET.** Layer-2 frame на проводе: `socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP))`, конструкция ARP request, парсинг ARP reply. Зачем ARP отдельно от ICMP (L2 vs L3).

**B-22 (atomic) — TCP SYN packet construction.** Собрать IP+TCP заголовок с `SYN`-флагом, посчитать checksum (включая псевдо-заголовок из B-18), отправить через raw socket. Не получать ответ — только отправка.

**B-23 (atomic) — SYN-ACK capture через pcap.** BPF фильтр `tcp[tcpflags] & (tcp-syn|tcp-ack) == (tcp-syn|tcp-ack) and src host <target>`, rate-limit, корреляция reply ↔ исходный SYN по `dst port`. Переиспользует мышцу из B-3.

**B-24 (atomic) — pthread pool + work queue.** Применяется к SYN scan pipeline: один поток шлёт SYN'ы (B-22), второй ловит SYN-ACK через pcap (B-23), третий агрегирует результаты. `pthread_mutex_t` для очереди, `pthread_cond_t` для wait/signal. Connect-scan остаётся однопоточным на epoll'е — это **мотивированный выбор**: SYN scan асимметричен (send/recv разделены), connect-scan нет.

**B-25 (integration) — host discovery.** ARP-sweep для локальной подсети (B-21) + ICMP-sweep для маршрутизируемой (B-20), выбор техники по сравнению target-сети и интерфейсов из B-16.

#### Магнум опус

**B-26 — network scanner.** Финальная сборка всех компонентов в один CLI инструмент. Отдельная репа, README, tests, CI. См. "Магнум опус (B-26)" выше.

---

### Блок C — Rust [параллельный трек]

**Формат блока — учебник-driven, линейно по The Book.** Одна глава (или пара тесно связанных глав) = одно задание `C-N`. Почти все атомарные: каждая глава вводит новые детали языка. Структура «4 атомарных + интеграционное + опус» к блоку C **не применяется** (см. скилл `plan-block` → исключение). Главы-проекты The Book (12 — minigrep, 20 — web server) играют роль естественных синтез-точек.

**Принцип задания:** прочитать главу → выделить новые концепты → написать маленькую программу, которая их реально задействует (по возможности в домене проекта — порты/сканер/сети, чтобы код накапливался по теме). После — конспект в `topics/rust/` + глоссарий English-терминов.

**Оставшаяся карта (The Book, edition 2024):**

| # | Главы | Тема | Артефакт |
|---|-------|------|----------|
| C-6 | 9 | error handling — `panic!` vs `Result`, `?`, `Box<dyn Error>`, `unwrap`/`expect`/`?`-propagation. Закрывает вопрос 4 из [[guessing-game-notes]] | мини-парсер с восстановлением ошибок |
| C-7 | 10 | generics, **traits** (определение/реализация/trait bounds/default methods), **lifetimes** (явные `'a`, почему компилятор требует) | дженерик-контейнер + свой trait |
| C-8 | 11 | automated tests — `#[test]`, `assert!`/`assert_eq!`, `cargo test`, `#[cfg(test)]`, организация тест-модулей | покрыть тестами один из прошлых проектов |
| C-9 | 12 | **проект-синтез: minigrep** — CLI-утилита (args, чтение файла, env vars, ошибки в stderr, разделение lib.rs/main.rs). Тянет главы 7–11 | `minigrep` |
| C-10 | 13 | functional features — **closures** (`Fn`/`FnMut`/`FnOnce`), **iterators** (`Iterator` trait, lazy, adapters `map`/`filter`/`collect`), zero-cost abstraction | переписать обработку из minigrep на итераторы |
| C-11 | 15 | smart pointers — `Box<T>`, `Rc<T>`, `RefCell<T>`, `Deref`/`Drop`, interior mutability, reference cycles | связный список / дерево |
| C-12 | 16 | **fearless concurrency** — `thread::spawn`, channels (`mpsc`), `Mutex<T>` + `Arc<T>`, маркеры `Send`/`Sync`. Мостик к C-параллелизму (pthreads) на безопасной модели | конкурентный счётчик / worker pool |

**Главы 14, 17–19** (More about Cargo / async / OOP-паттерны / patterns deep / advanced features — `unsafe`, advanced traits, макросы) — **по необходимости**, не обязательным потоком. async (tokio) и `nix`/`serde` системно разбираются уже в **блоке D**, поэтому здесь не дублируются. Глава 20 (multithreaded web server) — опциональный капстоун блока.

**Где заканчивается блок C:** базовый The Book освоен (ownership, типы данных, error handling, traits/generics/lifetimes, тесты, итераторы/замыкания, smart pointers, базовый concurrency). Дальше — блок D.

---

## Этап 2 — VPN (лето 2026 - зима 2026)

**Якорный проект: упрощённый VPN-клиент на Rust.** WireGuard-like туннель: UDP + шифрование трафика + tun-интерфейс. Не клон WireGuard — минимальная рабочая реализация с пониманием каждой строки.

**Что нужно освоить до начала:** Rust (ownership, error handling, async/tokio), криптография (ChaCha20-Poly1305, ECDH key exchange), сети (UDP tunneling, tun/tap interfaces), протокол (WireGuard whitepaper, ~15 страниц).

**Блок D — Rust intermediate.** Async Rust (tokio: сетевое программирование без блокировок), сериализация (serde + bincode для пакования структур по сети), системные вызовы из Rust (nix crate: tun/tap, raw sockets). Карта заданий — при старте блока.

**Блок E — Криптография.** Через Rust crates, не свои реализации: хеш-функции (SHA-256/BLAKE2, утилита хеширования файлов), AEAD-шифрование (ChaCha20-Poly1305 — то что в WireGuard), асимметрика (ECDH key exchange через x25519-dalek — как два узла договариваются о ключе не передавая его). Карта заданий — при старте блока.

**Блок F — VPN-проект.** Сборочная последовательность под якорный проект: TUN-интерфейс (читать/писать IP-пакеты) → UDP-туннель (проброс пакетов между двумя сетями/неймспейсами) → шифрование туннеля (ChaCha20-Poly1305) → key exchange (ECDH handshake при установке соединения) → финал: минимальный рабочий VPN между двумя VM. Карта заданий — при старте блока.

---

## Этап 3 — Изоляция и наблюдение (2027)

**Якорный проект: LLM-jail — sandbox для LLM-агентов с eBPF-телеметрией.** LLM-агент с tool use (LangChain/LlamaIndex) запускается в изолированном sandbox'е (namespaces + seccomp + cgroups). Все его действия (syscalls, открытие файлов, сетевые соединения) логируются через eBPF. Входы фильтруются prompt injection детектором, на выходе — DLP для секретов/PII. Пересечение трёх миров: kernel internals, observability, LLM-security — сильное портфолио для СЗИ-вакансий.

**Что нужно освоить:** Linux primitives изоляции (namespaces PID/mount/net/user, seccomp-bpf, cgroups, capabilities, pivot_root); eBPF (модель, верификатор, libbpf, ring buffer, kprobes/tracepoints); LLM-security application layer (prompt injection, agent security, DLP для output); threat modeling (STRIDE-like для LLM-агентов).

**Блок Z — Заход в LLM-security [перед блоком G].** Познакомиться с LLM-security как builder до того как этап 3 завязан на ней — если нравится, этап идёт уверенно; если нет, корректируем вектор заранее. Два задания: **Z1** — теория OWASP Top 10 for LLM Applications (prompt injection, insecure output handling, excessive agency и т.д.) → threat model на одностраничку в `topics/llm-sec/`. **Z2** — pet-project **prompt injection detector** на Python (1-2 недели, первая серьёзная Python-задача: type hints, venv/poetry, pytest, requests): слои детекции (regex-паттерны → эвристики → опционально классификатор на embeddings), корпус из открытых датасетов атак, метрики precision/recall, **отдельная репа** на GitHub. Подробная разбивка — при старте блока.

**Блок G — Sandbox: Linux primitives изоляции (C).** Здесь **в полной мере возвращаемся к темам блока A** (он закрыт по старой схеме). Блок G проектируется по новой структуре (16+4+1) и в интеграционных заданиях совмещает темы изоляции (namespaces, seccomp, cgroups, capabilities) с темами блока A (fork/exec, signals, pipes, pthreads, fd refcount, /proc) — спираль на новом уровне.

**Блок H — eBPF: observability и runtime security.** Kernel-side всегда на C (требование верификатора). User-space loader — на C (libbpf) или **Go** (libbpf-go / cilium/ebpf); Go опционально, но даёт входной билет в cloud-native security экосистему (Cilium, Tetragon, Falco, Trivy). Решение по языку — после H1.

**Блок I — LLM-security application layer.** В основном **Python** (де-факто язык LLM-экосистемы, уже подтянут через блок Z). Добавляются transformers, sentence-transformers, opentelemetry, FastAPI, Microsoft Presidio.

**Блок J — Финал этапа 3: LLM-jail.** Полиглот по дизайну: sandbox runner — C/Rust (G→J), eBPF kernel-side — C (H→J), eBPF user-side и tool-orchestrator — C/Rust/Go, gates (prompt injection, DLP) — Python, e2e обвязка и LangChain-агент — Python. Реальные production-системы выглядят именно так — это упражнение по интеграции стека.

---

## CTF

**Параллельно с этапом 1, начиная сейчас.** picoCTF — beginner, бесплатно, онлайн.
Темп: **1-2 челленджа в неделю**, 2-3 часа суммарно. Не превращать в основное занятие — это тренажёр и резюме-строитель, не цель.

Прогрессия категорий:
1. **General Skills** + **Forensics** — для разогрева, изучения утилит
2. **Web Exploitation** — параллельно с блоком B (синергия с OWASP Top 10)
3. **Binary Exploitation** + **Reverse Engineering** — параллельно с этапом 3 (глубокий стек)
4. **Cryptography** — параллельно с этапом 2 (после изучения Serious Cryptography)

Детальный алгоритм CTF-сессии — `.claude/skills/ctf/skill.md`.

---

## Принципы треков (детали в скиллах)

- **Мок-собесы** — **только по запросу пользователя** ("мок-собес", "проверь меня", "вопросы по пройденному"). Никаких счётчиков. Скилл `.claude/skills/mock-interview/skill.md`. Журнал — `roadmap/journal.md` → "Мок-собесы", детали — `knowledge-base/interviews/`.
- **Reading-code** — **инициирует Claude по контексту**. Естественные моменты: после интеграционного задания, как компонент мок-собеса, при переходе на новую технологию. Также по прямому запросу. Скилл `.claude/skills/reading-code/skill.md`. Журнал — `roadmap/journal.md` → "Reading-сессии".
- **CTF** — **пользователь сам следит за темпом**. Скилл срабатывает только по прямому запросу. Цель к концу 2026 (30+ челленджей) остаётся ориентиром.
- **Логика выбора задания** — `.claude/skills/give-task/skill.md`. Помимо темы определяет **позицию в блоке** (какое по счёту атомарное / интеграционное / магнум опус).
- **Обновление файлов после сессии** — `.claude/skills/session-debrief/skill.md`.
