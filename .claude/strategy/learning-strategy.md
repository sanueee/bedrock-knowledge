# Стратегия обучения — fn2s

Этот файл — **план обучения и принципы**. Что выполнено, где сейчас, слабые места — в диспетчере [knowledge-base/roadmap/00-roadmap.md](../../knowledge-base/roadmap/00-roadmap.md) → "Текущая позиция". Журналы заданий — `roadmap/journal.md`, пройденные темы — `roadmap/archive.md`.

Скиллы (give-task, mock-interview, reading-code, ctf, session-debrief) читают **roadmap/00-roadmap.md** для статуса и **этот файл** для плана/принципов.

---

## Главный вектор

**Сети → VPN-клиент → Изоляция и наблюдение (sandbox + eBPF + LLM-security)**

Навыки учатся не сами по себе — они учатся под конкретный проект. Каждый этап заканчивается живым якорным проектом.

| Этап | Якорный проект | Когда |
|------|---------------|-------|
| 1. Фундамент | Network scanner (C) + bonus: prompt injection detector (Python) | сейчас — конец 2026 |
| 2. VPN | Упрощённый VPN-клиент (Rust) | 2027 |
| 3. Изоляция и наблюдение | LLM-jail: sandbox для LLM-агентов с eBPF-телеметрией (C/Rust) | 2027–2028 |

**Цель:** первый оффер к лету–осени 2028 (конец 3-го курса).

### Языки в стратегии

**Основные** (под якорные проекты, осознанная глубина):
- **C** — этап 1 (Linux internals, сети, mini-firejail в этапе 3)
- **Rust** — этап 2 (VPN, потом части этапа 3)

**Инструментальные** (учим по необходимости, без отдельных учебных блоков):
- **Python** — везде где LLM-security и SAST: блок Z (prompt injection detector), блоки I и J этапа 3 (LLM-обвязка, NeMo Guardrails, llm-guard, Microsoft Presidio), DevSecOps-tooling (bandit, semgrep), CTF (pwntools для exploit dev). Прокачивается через эти задачи, не отдельным курсом.
- **Go** — для user-space части eBPF-инструментов (libbpf-go, Inspektor Gadget) и для чтения/патчинга чужого кода экосистемы (Cilium, Tetragon, Falco, Trivy). В блоке H можно опционально написать syscall-tracer на Go вместо C — это упростит user-space часть и даст входной билет в Go.

**Правило**: Python/Go не получают отдельных блоков и task'ов "учим язык". Они растут вместе с проектами. Когда упираемся в задачу где они нужны — `explain-code` объясняет минимально достаточный фрагмент языка, пишем, движемся дальше.

**Почему LLM-jail как финальный проект.** Это пересечение трёх стеков (Linux internals + observability + LLM-security), которое даёт уникальную точку дифференциации. Sandbox и eBPF по отдельности — стандартные инструменты разработчика СЗИ.

---

## Принципы

1. **Проект раньше языка** — не "учу Rust", а "пишу VPN на Rust". Язык — инструмент, проект — мотивация.
2. **Спираль, не линия** — темы возвращаются на новом уровне. Сначала "как использовать", потом "как работает внутри".
3. **Код раньше теории** — сначала написать и сломать, потом читать man.
4. **Одна атомарная задача — одна тема** — не смешивать `fork()` и `epoll` в одном атомарном задании.
5. **Каждый task завершается vault-записью** — иначе знание не зафиксировано.
6. **Глубина важнее скорости** — лучше второй раз пройти "то же самое" под другим углом, чем оставить ощущение неполноты. Интеграционные задания и магнум опус — не церемония, а механизм перехода от "видел" к "умею".
7. **Мок-собес и reading-code — не по счётчикам.** Мок-собес инициирует пользователь когда хочется проверить теорию. Reading-code инициирует Claude (естественно — после интеграционного задания или как компонент мок-собеса). CTF — пользователь сам следит.
8. **Английский — рабочий словарь** — термины и концепции в vault фиксируются на английском (с русским переводом). Будущие мануалы, RFC, собеседования — на английском.

---

## Структура блока

Базовая структура блока — **21 задание**:

- **16 атомарных** — одна тема на задание.
- **4 интеграционных** — на позициях 5, 10, 15, 20. Не новая теория, а **синтез последних 4 атомарных в одной задаче**. Цель — совместить уже пройденное в одном проекте под давлением. Это и есть переход от "видел" к "умею".
- **1 магнум опус** — финальное задание блока. Определяется в начале блока, к нему ведут все остальные. Сдаётся как отдельная репа на GitHub с README.

**Размер блока — гибкий.** 21 — базовый минимум, не потолок. Если глубина темы требует — блок растягивается (26, 31, 36...). Структура сохраняется: атомарные идут пачками по 4, **интеграционное — каждое 5-е задание**, магнум опус — последнее. Решение о размере принимается на сессии дизайна блока (карта тем → размер → опус), фиксируется в этом файле и в roadmap. Пример: блок B спроектирован на **26 заданий** (20 атомарных + 5 интеграционных + 1 опус) — глубина сетевого стека под якорный проект "network scanner" не уместилась в 16 атомарных слотов.

**Структура обязательна для каждого блока.** Любой блок, спроектированный по новой схеме, обязан иметь вид `(4k атомарных) + (k интеграционных, каждое 5-е) + 1 магнум опус` — то есть 21, 26, 31, 36... Блок без интеграционных и без опуса — это «закрытый задним числом по старой схеме» (как A), а не валидный новый блок. При дизайне блока этот инвариант проверяется явно.

**Исключение — учебник-driven блоки (Rust по The Book).** Блок C **намеренно идёт линейно по главам The Book** и таким и остаётся (переработки под 4+1-структуру не планируется): одна глава (или пара) = одно задание (C-1 = гл.1–2, C-4 = гл.6, C-5 = гл.7–8). Почти все такие задания — **атомарные**, потому что каждая глава вводит новые детали языка, а не синтезирует старое. Нумерация `C-N` означает «N-е Rust-задание», **не** «позиция N в фиксированной карте», поэтому правило «каждое 5-е интеграционное» к ней **не применяется**. Это сознательное отклонение для блоков, ведомых учебником, а не баг give-task. Синтез в таких блоках возникает естественно (задание по новой главе часто опирается на 2–3 пройденные), но отдельные интеграционные слоты и магнум опус для них не выделяются.

**Гейт закрытия блока:**
1. Магнум опус сдан.
2. **Blank-slate ретест** самой сложной темы блока — пройден без подсказок и без vault. Если не пишется с нуля — точечно возвращаемся, ретест повторяется. Только после прохождения — блок считается закрытым.

**Карта тем определяется до магнум опуса**, не наоборот. Иначе часть тем будет натянута под выбранный опус.

**Исключения:** структура — не догма для блоков, пройденных по старой схеме (см. блок A — закрыт задним числом, темы добираются в блоке G).

---

## Временный блок — Экзамен «Структуры данных» (deadline ~2026-06-20)

> **Вне основного вектора.** Это университетский экзамен 1 курса, не часть карьерной
> траектории (сети → VPN → изоляция). Он **time-boxed** (10 дней, с 2026-06-10) и после
> сдачи закрывается. Поэтому он **не подчиняется** структуре «16+4+1», не идёт через
> `give-task` и не ведётся в основном `roadmap/00-roadmap.md`.

**Зачем вообще в стратегии.** Структуры данных — фундамент, который всё равно нужен под
основной вектор (rbtree в `epoll`, хеш-таблицы в ядре, кучи в планировщиках, trie в
IDS/роутинге). Экзамен — повод прокачать теорию, которая потом окупится. Поэтому не
выбрасываем после сдачи, а оставляем конспекты как базу.

**Как ведётся:**
- Отдельный скилл **`/exam`** (`.claude/skills/exam/skill.md`), отдельный хаб-статус
  **`knowledge-base/topics/ds/00-index.md`** (аналог roadmap для этого блока).
- Конспекты — `topics/ds/NN-<name>.md`. Код (реализации на C) — `ds/` в корне репо.
- Формат экзамена (со слов препода): ~60 мин, скорее всего **тест** (варианты ответа /
  поле ввода, обратный таймер, вопросы разной стоимости, **последовательно без возврата**,
  случайная генерация на студента); не исключён классический **экзамен по билетам**.
- **Два пробных экзамена**: диагностический (середина, ~день 5–6) и генеральная репетиция
  (~день 9, полный формат с таймером).

**Упор — теория** (так просил пользователь), практика на C — закрепление для builder'а.
Принцип подачи следует **шаблону препода** (определение → пример → особенности →
плюсы/минусы → применение → операции → сложность), обогащённому параллелями Python/Rust и
реальными системами (для «блеснуть» на устном). Препод тяжёл на теорию: гоняет **формулы
мощности/размера**, **расчёт сложности с вероятностями/амортизацией**, **представление в
памяти** (машинное слово, row/column-major, упаковка/выравнивание) и **проблемы памяти**
(фрагментация внутр./внешн., висячие указатели, use-after-free). Детали — в скилле `/exam`.

**Карта тем подтверждена по реальному syllabus** (лекции Л1–Л3 есть в электронном виде) —
в `topics/ds/00-index.md`. Перечень структур: типы (простые/составные), массив, запись,
union, записи с вариантами, множество, последовательность, динамич. массив, линейный список,
стек, очередь, дека, деревья (BST, AVL, splay, B-, двоичные B-, красно-чёрные, оптимального
поиска), хеш-таблицы/ассоциативные массивы, графы.

---

## Этап 1 — Фундамент (сейчас — конец 2026, ~12 месяцев)

**Цель этапа:** знать Linux изнутри на уровне системных вызовов, понимать сетевой стек от Ethernet до TLS, написать первый Rust-код.

**Якорный проект: network scanner на C**
Сканер сети: ARP discovery + TCP port scan + определение сервиса по порту.
Не клон nmap — свой инструмент с пониманием каждой строки.
Все блоки A и B работают на него.

**Bonus-проект в конце этапа: prompt injection detector на Python**
1-2 недели после network scanner. Цель — войти в LLM-security как builder, не как исследователь. Подробнее в блоке Z.

**Параллельные треки на всё время этапа 1:**
- **CTF (picoCTF)** — 1-2 челленджа в неделю, начать прямо сейчас. Категории: General Skills, Forensics, потом Binary Exploitation/Reverse. Не превращать в основное занятие — это тренажёр, не цель. Детали категорий и приоритетов — раздел "## CTF" внизу.
- **OWASP Top 10 (web)** — пройти параллельно с блоком B (любой момент до B5/TLS — синергия с HTTP-темами). Статья + DVWA/WebGoat для практики. Место: `topics/security/owasp-top10.md`.
- **Reading-code** — реализуется внутри `mock-interview`, скилл `reading-code`. Также обязательно при завершении блока.

---

### Блок A — Linux Internals (C)

> **Статус: закрыт задним числом по старой схеме** (1 атомарное задание = 1 тема, 8 атомарных + Interview 01 reinforce, без интеграционных, без магнум опуса).
>
> Возврат в полной мере — в **блоке G** по новой структуре (21 задание). Темы блока A — `/proc`, fork/exec/wait, signals, pipes, pthreads, fd refcount — углубляются там через интеграционные задания вместе с темами изоляции (namespaces, seccomp, cgroups). Это и есть "спираль" из принципа 2.

#### A1 — /proc filesystem
Цель: читать и парсить /proc/\<pid\>/status.
Задание-шаблон: вывести список процессов с именем и RSS-памятью.
Функции: `opendir()`, `readdir()`, `fopen()`, `fgets()`, `snprintf()`, `fprintf()`

#### A2 — Сортировка: qsort
Цель: отсортировать список процессов по использованию памяти.
Задание-шаблон: вывести top-5 процессов по VmRSS.
Функции: `qsort()`, компаратор с `const void *`

#### A3 — Файловые дескрипторы
Цель: читать /proc/\<pid\>/fd/ и раскрывать симлинки.
Задание-шаблон: вывести список открытых файлов процесса.
Функции: `readlink()`, `opendir()`, `readdir()`

#### A4 — Процессы: fork/exec/wait
Цель: создавать дочерние процессы и управлять ими.
Задание-шаблон: fork + exec дочернего процесса + waitpid с корректной обработкой кода выхода.
Функции: `fork()`, `execv()`, `waitpid()`, `WEXITSTATUS()`

#### A5 — Сигналы: signal, kill, sigaction
Цель: понять межпроцессное взаимодействие через сигналы.
Задание-шаблон: написать программу с обработчиком SIGINT и SIGTERM, которая при получении сигнала пишет в лог и корректно завершается.
Функции: `sigaction()`, `sigset_t`, `kill()`
После: добавить обработку сигналов в предыдущий fork-проект.

#### A6 — Pipes и IPC
Цель: данные между процессами через pipe.
Задание-шаблон: родитель и дочерний процесс общаются через pipe — дочерний пишет, родитель читает.
Функции: `pipe()`, `dup2()`, `read()`, `write()`

#### A7 — /proc/net: чтение сетевого состояния
Цель: мост между Linux internals и сетями.
Задание-шаблон: прочитать /proc/net/tcp, распарсить hex-адреса и порты, вывести список активных TCP-соединений.
Функции: fopen/fgets + strtol для hex-парсинга
Место: `linux/proc/` для кода, `topics/linux/` и `topics/networking/` для теории

#### A8 — Потоки: pthreads
Цель: параллелизм на уровне потоков.
Задание-шаблон: параллельный сбор данных из /proc — несколько потоков читают разные PID, результат в общий массив с мьютексом.
Функции: `pthread_create()`, `pthread_join()`, `pthread_mutex_t`

#### Interview 01 — 2026-04-28

**Результат:** с пробелами

**Уверенно:** зомби-процесс (термин), little-endian в /proc/net/tcp, data race (суть)

**Слабые места:**
- `opendir()` vs `readdir()`: перепутал кто из них возвращает NULL при ошибке, а кто при конце директории
- Зомби-процесс: не знал что проблема в таблице процессов / PID, а не в дескрипторах
- `sigaction()` vs `signal()`: не знал про автосброс обработчика и `sa_mask`
- Pipe EOF: не знал что `read()` зависнет если не закрыть write-end у читателя

**Закрепляющие задания:**
- `opendir()` на несуществующем пути → `errno`/`perror()`, потом `readdir()` до NULL
- `fork()` + дочерний `exit`, родитель спит без `waitpid()` → зомби в `ps aux | grep Z`
- `signal()` vs `sigaction()` с `SA_RESTART` — увидеть разницу поведения
- pipe + не закрывать `fd[1]` у родителя → `read()` виснет; `close(fd[1])` → EOF

---

### Блок B — Сетевой стек (C)

> **Структура: 26 заданий** (20 атомарных + 5 интеграционных + 1 магнум опус). Спроектировано 2026-05-25.
>
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

#### Фаза 1 — TCP базис и наблюдение

**B-1 (atomic) — OWASP Top 10 (web): теория.** ✅ task-b01.
Знать на пальцах SQLi/XSS/SSRF/IDOR/Broken Auth/CSRF и остальные. Параллельный трек, синергия с HTTP-темами фазы 3. Место: `topics/security/owasp-top10.md`.

**B-2 (atomic) — TCP-сокеты: echo server + client.** ✅ task-b02.
`socket`/`bind`/`listen`/`accept`/`connect`/`send`/`recv`, `SO_REUSEADDR`, partial read на stream, `EINTR`/`EPIPE`/`SIGPIPE` через `SIG_IGN`. Конспект: [[tcp-sockets]].

**B-3 (atomic) — libpcap sniffer.** ✅ task-b03.
pcap lifecycle, BPF фильтр в ядре, парсинг Ethernet/IPv4/TCP байт за байтом, переменная длина заголовков. Конспекты: [[libpcap]], [[ethernet-frame]].

**B-4 (atomic) — getaddrinfo + DNS resolution.** ← следующее.
Resolve hostname в `sockaddr`, addrinfo linked list, `gai_strerror`, IPv4/IPv6 abstraction. Паттерн "свой namespace ошибок". Заготовка конспекта уже есть: [[getaddrinfo]].

**B-5 (integration) — `host-probe`.** Резолв имени через getaddrinfo → TCP connect → recv баннера → печать. Синтез B-2 (sockets), B-3 (понимание layer), B-4 (resolve).

#### Фаза 2 — Non-blocking I/O и connect-scan

**B-6 (atomic) — Non-blocking I/O.** `fcntl(O_NONBLOCK)`, `EAGAIN`/`EWOULDBLOCK`, `EINPROGRESS` на connect, `SO_RCVTIMEO`/`SO_SNDTIMEO` как альтернатива.

**B-7 (atomic) — Multiplexing: epoll.** Сравнение `select`/`poll`/`epoll`, edge-triggered vs level-triggered, `epoll_ctl`/`epoll_wait`, `EPOLLIN`/`EPOLLOUT`/`EPOLLERR`. Заготовка конспекта: [[event-loop-epoll]].

**B-8 (atomic) — Connect-with-timeout pattern.** Non-blocking connect → epoll на `EPOLLOUT` → `getsockopt(SO_ERROR)` для проверки успеха. Классический pattern для port scanner'а.

**B-9 (atomic) — UDP sockets.** `recvfrom`/`sendto`, connectionless model, отсутствие partial read (datagram-границы), потеря пакетов как штатное событие.

**B-10 (integration) — port-scan v1.** Connect-scan диапазона портов одного хоста: epoll + non-blocking connect + тайм-ауты + сводка open/closed/filtered. Синтез B-6/B-7/B-8.

#### Фаза 3 — Application protocols + service detection

**B-11 (atomic) — HTTP basics.** GET/HEAD запрос, status line, `Server:` header, `Content-Length` vs chunked transfer. Минимальный клиент достаточный для banner grab.

**B-12 (atomic) — DNS protocol manually.** Построить A-record query поверх UDP, парсить ответ, **компрессия имён обязательна** (без неё парсинг реальных ответов фейкнет). Сравнить с getaddrinfo из B-4.

**B-13 (atomic) — Service fingerprinting.** Библиотека пробов: SSH banner (server присылает первым), HTTP HEAD на 80/8080/8443, SMTP greeting на 25/587, FTP banner на 21. Таблица "порт → проба → паттерн".

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

**B-26 — network scanner.** Финальная сборка всех компонентов в один CLI инструмент. Отдельная репа, README, tests, CI. См. "Магнум опус" выше.

#### Interview 03 — 2026-06-09

**Scope:** блок B (B-1…B-6), удержание. **Результат:** с пробелами (механику держит, модель blocking/non-blocking — гэп).

**Уверенно:** три исхода `recv` (`0`=FIN); `ip_hl × 4` (переменная длина, self-describing); каст `sockaddr` по `ai_family` (type tag, выбор схемы чтения); `EAGAIN` vs `EINTR` (противоположные реакции); `getsockopt(SO_ERROR)` после `poll(POLLOUT)` — readiness ≠ correctness, async-канал ошибки (разобрал отлично).

**Слабые места:**
- **Что делает сокет блокирующим** — путает: «таймаут (`SO_RCVTIMEO`) делает сокет блокирующим». На деле blocking — режим по умолчанию; `SO_RCVTIMEO` лишь ограничивает уже существующую блокировку. Режим задаётся `O_NONBLOCK` (fcntl), не таймаутом.
- **Исход «hang vs мгновенная сдача» при убранном `poll`** — считает недетерминированным («что угодно»). На деле жёстко задан режимом: non-blocking → `recv` `EAGAIN` мгновенно → выход без данных (не hang); hang — только blocking без таймаута. Матрица 2×2 не автоматизм.
- Терминология: `EINTR` = прерван **сигналом**, не «сисколлом».

**Закрепляющие задания:**
- `sockmode_demo` ([[task-b-interview03-reinforce]]) — один `recv` на молчащем сервере в трёх конфигурациях: (1) blocking + `SO_RCVTIMEO(1s)` → блок ~1с → `EAGAIN`; (2) non-blocking (`O_NONBLOCK`) → `EAGAIN` мгновенно (~0мс); (3) blocking без таймаута под `alarm(2)` → виснет, пока `SIGALRM` не прервёт (`EINTR`). Замерять elapsed по каждому (`clock_gettime`/`gettimeofday`) и печатать. Цель — **тактильно** ощутить «кто ждёт и сколько». Хедеры: `<sys/socket.h>`, `<fcntl.h>`, `<sys/time.h>`, `<unistd.h>` (`alarm`), `<signal.h>`, `<errno.h>`, `<time.h>`. Закрыть до B-7, в связке с `/theory` по [[nonblocking-poll]].

---

### Блок C — Rust [параллельный трек, доступен после B1]

**Формат блока — учебник-driven, линейно по The Book.** Одна глава (или пара тесно связанных глав) = одно задание `C-N`. Почти все атомарные: каждая глава вводит новые детали языка. Структура «4 атомарных + интеграционное + опус» к блоку C **не применяется** (см. "## Структура блока" → исключение). Главы-проекты The Book (12 — minigrep, 20 — web server) играют роль естественных синтез-точек вместо формальных интеграционных слотов. Статус каждого задания — в roadmap, не здесь.

**Принцип задания:** прочитать главу → выделить новые концепты → написать маленькую программу, которая их реально задействует (по возможности в домене проекта — порты/сканер/сети, чтобы код накапливался по теме, а не был «hello-примерами»). После — конспект в `topics/rust/` + глоссарий English-терминов.

**Карта заданий (The Book, edition 2024):**

| # | Главы | Тема | Артефакт |
|---|-------|------|----------|
| C-1 | 1–2 | toolchain (rustup/rustc/cargo, Cargo.toml/lock, crate, edition), базовый синтаксис | `hello_world`, `guessing_game` |
| C-2 | 3–4 | типы, `let`/`mut`/shadowing, statements vs expressions, control flow; **ownership** (move/Copy, `&T`/`&mut T`, slices, dangling) | `word_tools` |
| C-3 | 5 | structs (named/tuple/unit), `impl`, receiver (`&self`/`&mut self`/`self`), associated fn vs method, `#[derive(Debug)]` | `rectangles` |
| C-4 | 6 | enums с данными, `match` как выражение (exhaustive), `Option<T>` вместо null, `if let` | `scan_report` |
| C-5 | 7–8 | modules/packages/paths/`use`; collections (`Vec`/`String`/`HashMap`) | `scan_aggregator` |
| C-6 | 9 | error handling — `panic!` vs `Result`, `?`, `Box<dyn Error>`, `unwrap`/`expect`/`?`-propagation. Закрывает вопрос 4 из [[guessing-game-notes]] | мини-парсер с восстановлением ошибок |
| C-7 | 10 | generics, **traits** (определение/реализация/trait bounds/default methods), **lifetimes** (явные `'a`, почему компилятор требует) | дженерик-контейнер + свой trait |
| C-8 | 11 | automated tests — `#[test]`, `assert!`/`assert_eq!`, `cargo test`, `#[cfg(test)]`, организация тест-модулей | покрыть тестами один из прошлых проектов |
| C-9 | 12 | **проект-синтез: minigrep** — CLI-утилита (args, чтение файла, env vars, ошибки в stderr, разделение lib.rs/main.rs). Тянет главы 7–11 | `minigrep` |
| C-10 | 13 | functional features — **closures** (`Fn`/`FnMut`/`FnOnce`), **iterators** (`Iterator` trait, lazy, adapters `map`/`filter`/`collect`), zero-cost abstraction | переписать обработку из minigrep на итераторы |
| C-11 | 15 | smart pointers — `Box<T>`, `Rc<T>`, `RefCell<T>`, `Deref`/`Drop`, interior mutability, reference cycles | связный список / дерево |
| C-12 | 16 | **fearless concurrency** — `thread::spawn`, channels (`mpsc`), `Mutex<T>` + `Arc<T>`, маркеры `Send`/`Sync`. Мостик к C-параллелизму (pthreads) на безопасной модели | конкурентный счётчик / worker pool |

**Главы 14, 17–19** (More about Cargo / async / OOP-паттерны / patterns deep / advanced features — `unsafe`, advanced traits, макросы) — **по необходимости**, не обязательным потоком. async (tokio) и `nix`/`serde` системно разбираются уже в **блоке D (Rust intermediate)**, поэтому здесь не дублируются. Глава 20 (multithreaded web server) — опциональный капстоун блока, если захочется собрать всё пройденное в один проект перед переходом к D.

**Где заканчивается блок C:** базовый The Book освоен (ownership, типы данных, error handling, traits/generics/lifetimes, тесты, итераторы/замыкания, smart pointers, базовый concurrency). Дальше — блок D (intermediate: tokio, serde, nix) под VPN этапа 2.

#### Interview 02 — 2026-06-04

**Результат:** с пробелами (по запросу перед C-5, главы 1–8). Детали — [[interview-02]].

**Уверенно:** `Option<T>` vs null (отсутствие в системе типов), правило заимствования `&mut` XOR `&`, владение в циклах `for x in v` (consume) vs `&v` (borrow).

**Слабые места:**
1. Формы receiver — `&mut self` считает «становишься владельцем» (на деле exclusive borrow); `self` by value считает shadowing (на деле move/consume).
2. Почему `String` не `Copy` — не назвал связку shallow copy → два владельца → double free. Доп: `.clone()`/`Clone` это std (не отдельный crate); `&str` — fat pointer, не «строка на стеке».
3. `match` exhaustiveness — не знал что забытый вариант = compile error `E0004` (не runtime). Смазана граница statement vs expression.
4. `v[i]` vs `v.get(i)` — думал что `v[i]` паникует на компиляции (на деле runtime; компиляция проходит).

**Закрепляющие задания:** отдельный reinforce-task не заводится. Пробелы 3–4 + Option/get закрываются прямо в C-5 (`scan_aggregator`). Пробелы 1–2 — мини-разминка в начале [[task-c05-rust-ch7-8-modules-collections]] (перечитать [[structs-methods]], [[ownership]] + микро-проверка перед кодом).

---

### Блок Z — Заход в LLM-security [конец этапа 1, перед этапом 2]

**Зачем:** познакомиться с LLM-security как builder до того как этап 3 завязан на ней. Если нравится — этап 3 идёт уверенно. Если нет — корректируем стратегию заранее.

#### Z1 — OWASP Top 10 for LLM Applications: теория
Цель: знать ключевые атаки. Prompt injection (LLM01), insecure output handling (LLM02), training data poisoning (LLM03), model DoS (LLM04), supply chain (LLM05), sensitive info disclosure (LLM06), insecure plugin design (LLM07), excessive agency (LLM08), overreliance (LLM09), model theft (LLM10).
Формат: статья + threat model на одностраничку в vault.
Место: `topics/llm-sec/owasp-llm-top10.md`.

#### Z2 — Pet-project: prompt injection detector
1-2 недели на **Python**. Это первая серьёзная Python-задача после школьного уровня — параллельно с детектором подтянуть Python: type hints, virtualenv/poetry, pytest, requests, базовые dataclasses.
Что делает: на вход — пользовательский prompt, на выход — флаг "подозрительно / чисто" + причина.
Слои детекции:
- regex-паттерны на классические injection-фразы ("ignore previous instructions", "you are now ...")
- эвристики: соотношение спец-токенов, аномальная длина, multi-language switch
- (опционально) лёгкий классификатор на embeddings (sentence-transformers + logistic regression)

Корпус: открытые датасеты атак (например, Lakera Gandalf prompts, JailbreakBench).
Тестовая часть: precision/recall на размеченном датасете.
Выложить в **отдельную репу** на GitHub с README, лицензией, парой тестов.

Место: `llm-sec/prompt-injection-detector/`, vault: `tasks/task-NN-llm-pi-detector.md`.

---

## Этап 2 — VPN (2027, ~10 месяцев)

**Якорный проект: упрощённый VPN-клиент на Rust**
WireGuard-like туннель: UDP + шифрование трафика + tun-интерфейс.
Не клон WireGuard — минимальная рабочая реализация с пониманием каждой строки.

**Что нужно освоить до начала:**
- Rust: ownership, error handling, async (tokio)
- Криптография: ChaCha20-Poly1305, ECDH key exchange
- Сети: UDP tunneling, tun/tap interfaces
- Протокол: WireGuard whitepaper (15 страниц)

---

### Блок D — Rust intermediate

#### D1 — Async Rust: tokio
Цель: сетевое программирование без блокировок.
Проект: TCP-сервер на tokio который обрабатывает несколько соединений.

#### D2 — Сериализация: serde + bincode
Цель: пакование структур для передачи по сети.

#### D3 — Rust + системные вызовы (nix crate)
Работа с tun/tap, raw sockets из Rust.

---

### Блок E — Криптография (читать "Serious Cryptography" параллельно)

#### E1 — Хеш-функции: SHA-256, BLAKE2
Написать утилиту хеширования файлов на Rust (без своей реализации — через sha2 crate).
Цель: понять что такое хеш и зачем.

#### E2 — Симметричное шифрование: ChaCha20-Poly1305
Зашифровать/расшифровать файл используя chacha20poly1305 crate.
Цель: понять AEAD-шифрование — то что используется в WireGuard.

#### E3 — Асимметрика: ECDH key exchange
Написать простой key exchange между двумя процессами через x25519-dalek.
Цель: понять как два узла договариваются о ключе не передавая его.

---

### Блок F — VPN-проект

#### F1 — TUN-интерфейс: читать/писать пакеты
Открыть tun-интерфейс, читать IP-пакеты, печатать src/dst.
Библиотека: tun crate

#### F2 — UDP туннель: проброс пакетов между двумя машинами
Два процесса на одной машине (разные сети/неймспейсы): пакет уходит через tun, приходит на другой стороне.

#### F3 — Шифрование туннеля
Добавить ChaCha20-Poly1305 к UDP туннелю из F2.

#### F4 — Key exchange при установке соединения
ECDH handshake перед началом передачи данных.

#### F5 — Финал: минимальный рабочий VPN
Две VM (VMware), трафик идёт через зашифрованный туннель.
Место: `networking/vpn/`

---

## Этап 3 — Изоляция и наблюдение (2027–2028)

**Якорный проект: LLM-jail — sandbox для LLM-агентов с eBPF-телеметрией**
LLM-агент с tool use (например LangChain/LlamaIndex) запускается в изолированном sandbox'е (namespaces + seccomp + cgroups). Все его действия (системные вызовы, открытие файлов, сетевые соединения) логируются через eBPF. Входы фильтруются prompt injection детектором. На выходе — DLP для секретов/PII.

Это пересечение трёх миров: kernel internals, observability, LLM-security. Никто из 3 курса такого не делает — это сильное портфолио для СЗИ-вакансий.

**Что нужно освоить:**
- Linux primitives изоляции: namespaces (PID/mount/net/user), seccomp-bpf, cgroups, capabilities, pivot_root
- eBPF: модель, верификатор, libbpf, ring buffer, kprobes/tracepoints
- LLM-security application layer: prompt injection, agent security, DLP для LLM-output
- Threat modeling: STRIDE-like разбор для LLM-агентов

---

### Блок G — Sandbox: Linux primitives изоляции (C)

> **В этом блоке возвращаемся в полной мере к темам блока A** (он был закрыт по старой схеме без интеграционных и без магнум опуса). Блок G проектируется по новой структуре (16+4+1) и в интеграционных заданиях вынужденно совмещает темы изоляции (namespaces, seccomp, cgroups, capabilities) с темами блока A (fork/exec, signals, pipes, pthreads, fd refcount, /proc). Это и есть "спираль на новом уровне".
>
> Существующие пункты G1–G6 ниже — старая разбивка, **референс до проектирования новой карты** перед началом блока.

#### G1 — Namespaces: первый jail
Цель: запустить процесс в собственных PID/mount/net namespace.
Функции: `clone()` с `CLONE_NEW*`, `unshare()`.
Задание-шаблон: запустить `/bin/sh` в новом PID namespace — убедиться что `ps` показывает только себя.

#### G2 — pivot_root и mount namespace
Цель: дать процессу собственную файловую систему (минимальный rootfs).
Функции: `mount()`, `pivot_root()`, `umount2()`.

#### G3 — seccomp-bpf: фильтр сисколлов
Цель: разрешить процессу только whitelist сисколлов.
Библиотека: `libseccomp` (потом понять как написать BPF-фильтр вручную).

#### G4 — Capabilities: дробление root-прав
Цель: дропнуть все capabilities кроме нужных.
Функции: `prctl(PR_CAPBSET_DROP, ...)`, `cap_set_proc()`.

#### G5 — cgroups v2: лимиты ресурсов
Цель: ограничить процесс по памяти, CPU, pids.
Через файловую систему: `/sys/fs/cgroup/`.

#### G6 — Промежуточный проект: mini-firejail
Объединить G1–G5 в утилиту: `./jail --memory 100M --syscalls read,write,exit ./target_program`.
~500-700 строк на C. Свой mini Firejail.
Место: `sandbox/mini-firejail/`. **Отдельная репа на GitHub с README.**

---

### Блок H — eBPF: observability и runtime security

**Стек user-space:** kernel-side всегда на C (это требование eBPF-верификатора). User-space loader можно писать на C (libbpf) или **Go** (libbpf-go / cilium/ebpf). Go в этом блоке — опционально, но даёт входной билет в Go-экосистему cloud-native security (Cilium, Tetragon, Falco, Trivy). Решение принимаем после H1 в зависимости от настроения.

#### H1 — Первая eBPF-программа: hello world
Цель: понять модель — user-space loader + kernel-space программа.
Инструмент: libbpf + clang. Загрузить программу, которая срабатывает на `execve`.

#### H2 — kprobes / tracepoints: трейс сисколлов
Цель: ловить события в ядре и передавать в user-space.
Механика: ring buffer / perf buffer для передачи событий.

#### H3 — Карты (BPF maps): хранение состояния
Цель: считать события на per-pid основе, агрегировать.
Типы: `BPF_MAP_TYPE_HASH`, `BPF_MAP_TYPE_RINGBUF`.

#### H4 — XDP/TC: сетевая обработка
Цель: фильтровать/инспектировать пакеты на уровне драйвера. Только обзорно — не углубляться.

#### H5 — Промежуточный проект: syscall-tracer
Утилита: `./tracer --pid <PID>` → лог всех `execve`, `open`, `connect` процесса с аргументами.
Аналог: упрощённый `execsnoop` + `opensnoop` + `tcpconnect` из bcc.
Стек: kernel-side — C; user-side — на выбор C (libbpf) или **Go** (cilium/ebpf). Если выбираем Go — это будет первый серьёзный Go-проект, прокачка идёт параллельно (стандартная библиотека, channels, context, cobra для CLI).
Место: `ebpf/syscall-tracer/`. **Отдельная репа на GitHub.**

---

### Блок I — LLM-security application layer

**Стек:** в основном **Python** (это де-факто язык LLM-экосистемы). К этому моменту Python уже подтянут через блок Z. В блоке I добавляются: transformers, sentence-transformers, opentelemetry, FastAPI, Microsoft Presidio.

#### I1 — OWASP Top 10 for LLM (углублённо)
Уже знаком из блока Z. Углубление: для каждой категории — атакующий пример + защитный паттерн.

#### I2 — Prompt injection detection: refresh + hardening
Доработать детектор из блока Z до production-ready: тесты, CI, документация. Возможно — добавить классификатор на base-модели через transformers.

#### I3 — Agent security: tool use threat model
Прочитать NeMo Guardrails / OpenAI plugin guidelines / LangChain security docs. Threat model для агента с tool use: что ломается, как защищать.
Место: `topics/llm-sec/agent-threat-model.md`.

#### I4 — Output sanitization / LLM DLP
Утилита-фильтр LLM-вывода: детектит секреты (API keys, JWT, PII) через regex + Microsoft Presidio. Удаляет/маскирует.
Место: `llm-sec/llm-dlp/`. **Отдельная репа.**

---

### Блок J — Финал этапа 3: LLM-jail

**Полиглот по дизайну:** sandbox runner — C/Rust (G6 → J2), eBPF kernel-side — C (H → J5), eBPF user-side и tool-orchestrator — C/Rust/Go, gates (prompt injection, DLP) — Python, e2e обвязка и LangChain-агент — Python. Реальные production-системы выглядят именно так — это и есть упражнение по интеграции стека.

#### J1 — Архитектура и threat model
Один документ: что защищаем, от кого, какие границы доверия. Без этого код будет хаотичным.

#### J2 — Sandbox runner для LLM-агента
Берём `mini-firejail` из G6, адаптируем: дополнительно мониторим всё что делает процесс (через eBPF из H5), пробрасываем строго ограниченный network namespace, ограничиваем capabilities.

#### J3 — Prompt injection gate на входе
Перед передачей пользовательского ввода в LLM — прогон через детектор из I2. Подозрительные запросы блокируются или помечаются.

#### J4 — DLP gate на выходе
Перед возвратом ответа LLM пользователю — прогон через DLP из I4.

#### J5 — Tool execution observability
Каждый tool call агента → логируется eBPF-трейсером. Анализ: что вызвал, к каким файлам обращался, какие сетевые соединения открывал.

#### J6 — Финал: e2e демо
LangChain-агент в твоём sandbox'е решает реальную задачу, твоя обвязка ловит попытки jailbreak'а, eBPF-логи показывают что именно делал агент. README + видео-демо + блог-пост.
Место: `llm-sec/llm-jail/`. **Главный pet-project портфолио.**

---

## CTF

**Параллельно с этапом 1, начиная сейчас.** picoCTF — beginner, бесплатно, онлайн.
Темп: **1-2 челленджа в неделю**, 2-3 часа суммарно. Не превращать в основное занятие — это тренажёр и резюме-строитель, не цель.

Прогрессия категорий:
1. **General Skills** + **Forensics** — для разогрева, изучения утилит
2. **Web Exploitation** — параллельно с блоком B (синергия с OWASP Top 10)
3. **Binary Exploitation** + **Reverse Engineering** — параллельно с этапом 3 (глубокий стек)
4. **Cryptography** — параллельно с этапом 2 (после изучения Serious Cryptography)

Цель к концу 2026: **30+ решённых челленджей**.
Если зайдёт по-крупному — пробовать командные CTF-ы (CTFTime: VolgaCTF, RuCTF), 1 в семестр.

Каждый решённый нетривиальный челлендж → запись в `knowledge-base/ctf/<name>.md` с writeup'ом. Это и тренировка письменного английского (writeup лучше делать на нём), и материал для будущих собеседований.

Детальный алгоритм CTF-сессии — `.claude/skills/ctf/skill.md`.

---

## Принципы треков (детали в скиллах)

- **Мок-собесы** — **только по запросу пользователя** ("мок-собес", "проверь меня", "вопросы по пройденному"). Никаких счётчиков. Скилл `.claude/skills/mock-interview/skill.md`. Журнал — `roadmap/journal.md` → "Мок-собесы".
- **Reading-code** — **инициирует Claude по контексту**. Естественные моменты: после интеграционного задания (читаем чужую реализацию схожих интеграций), как компонент мок-собеса (читаем модуль → обсуждаем), при переходе на новую технологию. Также по прямому запросу пользователя. Скилл `.claude/skills/reading-code/skill.md`. Журнал — `roadmap/journal.md` → "Reading-сессии".
- **CTF** — **пользователь сам следит за темпом**. Скилл срабатывает только по прямому запросу. Цель к концу 2026 (30+ челленджей) остаётся ориентиром.
- **Логика выбора задания** — `.claude/skills/give-task/skill.md`. Помимо темы определяет **позицию в блоке** (какое по счёту атомарное / интеграционное / магнум опус).
- **Обновление файлов после сессии** — `.claude/skills/session-debrief/skill.md`.

---

## Карьерная стратегия (обновлено 2026-04-29)

### Конечная цель: Разработчик СЗИ

**Что это:** пишут сами инструменты защиты — агенты, сканеры, драйверы, антивирусные движки.
Это builder-вакансия в чистом виде. C и Rust там реально нужны.
**Компании:** Kaspersky, Код Безопасности, InfoWatch, ИнфоТеКС.
**Сложность входа:** высокая. Junior-позиций мало, но стажёры бывают.

Это сложно, но именно сюда целимся. Не менять ориентир без прямого разговора с пользователем.

### Точка входа: Стажёр DevSecOps / AppSec (2-3 курс, ~2026-2027)

**Что это:** встраивание безопасности в CI/CD пайплайны — запуск SAST, подсчёт SBOM, блокировка небезопасных сборок. Хорошая точка входа без коммерческого опыта.
**Компании:** Ozon, Самокат, банки.

**Важно:** DevSecOps — это стартовая площадка, не конечный пункт. Не застрять в операционной работе. Системное программирование (fn2s) продолжается параллельно.

### Навыки ИБ-направления (вспомогательные, для DevSecOps-стажировки)

Не основной roadmap, но обязательные для стартовой позиции. Встраиваются в существующие блоки:

| Навык | Когда | Как |
|-------|-------|-----|
| OWASP Top 10 (web) | Блок B0 | Статья + DVWA/WebGoat (3-5 челленджей) |
| OWASP Top 10 for LLM | Блок Z1 / I1 | Статья + threat model в vault |
| picoCTF | Параллельно с этапом 1 | 1-2 челленджа в неделю, 30+ к концу 2026 |
| SAST | Блок B-C | semgrep / bandit (Python) на своём коде, cargo-clippy на Rust |
| CI/CD (GitHub Actions) | Блок B | Добавить pipeline к network scanner: build + clang-tidy + tests |
| SBOM | Блок C / D | cargo-sbom для своего Rust-проекта |
| Reading-code | Каждые 4 task'а | Скилл `reading-code` |
| Python (как инструмент) | Блок Z, I, J | Прокачивается через LLM-проекты, не отдельным курсом |
| Go (как инструмент) | Блок H/J опционально | Прокачивается через user-space часть eBPF-инструментов |

### Реальный мост к СЗИ-разработчику

Путь не через DevSecOps-операции, а через:
1. Глубокое системное программирование (текущий fn2s roadmap, этапы 1-2)
2. Несколько pet-projects на C/Rust на GitHub: network scanner, VPN, mini-firejail, syscall-tracer, llm-jail
3. Знание OWASP (web и LLM) как теоретическая база
4. Уникальная специализация: пересечение Linux internals + LLM-security (этап 3)

DevSecOps стажировка = деньги + опыт + рыночный старт. Не = путь к СЗИ.

### Почему LLM-security именно как specialization

К 2027-2028 ИБ-индустрия будет в острой фазе адаптации к LLM-агентам. Большинство security-специалистов — либо классические AppSec без понимания LLM, либо ML-инженеры без понимания изоляции и kernel-level threat'ов. Builder, который умеет одновременно:
- писать sandbox на C/Rust
- инструментировать процесс через eBPF
- понимать prompt injection и agent threat model

— это редкая комбинация. Это **точка дифференциации**, которая отличает резюме среди студентов 3 курса. Не "ещё один человек умеющий C", а "человек, который собрал работающий LLM-jail".
