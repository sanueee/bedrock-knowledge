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

Полный курс структур данных на C под экзамен (10 дней, 2026-06-10 … 2026-06-20). **Хаб со статусом, картой тем и журналом пробников — `topics/ds/00-index.md`** (primary source), конспекты — `topics/ds/NN-*.md`, код — `sections/ds/`. «Пройдено ≠ мастерство» — перед опорой на тему перечитать конспект в `topics/ds/`.

---

## Блок B — Сетевой стек (C) — активный (детальный дамп)

> Блок активен — дамп живёт здесь, диспетчер держит только позицию/следующий шаг/слабые места.

**Пройденные темы** (суть в 1–2 предложениях; детали — в конспектах по ссылкам):

- **B-17** (атомарное, Фаза 4 — Raw sockets + L2/L3) — Raw sockets и capabilities (`rawcap.c`): кто имеет право открыть `SOCK_RAW` и как держать это право минимально коротко. **Capabilities** = дробление root на ~40 битов; `CAP_NET_RAW` = право на raw/packet socket (спуфинг source IP → reflection-DoS, сниффинг, инъекция → потому gated). **Наборы**: permitted (склад, потолок) / effective (в руках, **его ядро проверяет в момент syscall**) / inheritable (переживёт `execve`, по умолчанию пуст). **Центральная идея — minimum capability surface**: `CAP_NET_RAW` проверяется **один раз** в `socket()`; fd выдан → операции над ним право не перепроверяют → паттерн «открыть raw socket → сразу `capng_clear`+`capng_apply` → старый fd жив, новый `socket()` даёт `EPERM`». Окно с опасным правом сжато до одной строки. **Три способа выдать право**: `sudo` (весь root) / setuid-root (весь root, хуже) / **`setcap cap_net_raw+ep`** (одна capability, uid обычный = least privilege); `+ep` = файловые permitted (`p` → permitted процесса при execve) + effective-флаг (`e` → сразу поднять в effective, чтобы наивная утилита не звала `capng_update`). **libcap-ng**: `capng_get_caps_process` (загрузить в буфер, ПЕРВЫМ) → `capng_have_capability(CAPNG_EFFECTIVE,...)` (читает буфер, не процесс) → `capng_clear` (**`void`, только буфер**) → `capng_apply` (**коммит в процесс**). Уроки `/check` (3 итерации): перевёрнутая логика второго `socket()` (EPERM = успех демонстрации, а не `exit(FAILURE)`); ветка «второй сокет открылся» = провал minimum surface, надо сигналить + не течь `new_fd`; `capng_apply` возврат обязателен (без него сброс фиктивный). **Главный баг — только прогон**: `CAPNG_SELECT_BOTH` (caps+bounding set) требует `CAP_SETPCAP` для сброса bounding → под `setcap`+nobody `apply` падает (`error: capng_apply`), рушится **ровно целевой** сценарий; фикс — `CAPNG_SELECT_CAPS`. Тест 3 поймал невидимое по чтению (`_BOTH` выглядел «полнее»). Docker: три ветки привилегий (nobody/root/setcap+nobody) все зелёные. Практика: bind-mount с macOS не хранит xattr `security.capability` → бинарник для `setcap` в контейнерную ФС. Слабое место (сам): механика простая, но **вывести сценарии привилегий и нужные проверки самому** — тяжело (перебор ветвей нагружает, как B-13/B-15). → [[capabilities]].
- **B-16** (атомарное, первое в Фазе 4 — Raw sockets + L2/L3) — Interface enumeration (`ifenum.c`): откуда сканер знает свою подсеть и выход наружу. **Два независимых источника ядра, сшивка по имени интерфейса**: `getifaddrs()` (адреса+маски) + `/proc/net/route` (маршруты). **`getifaddrs`**: связный список `struct ifaddrs` — **один узел = один адрес, не интерфейс** (`eth0` с IPv4+IPv6+MAC = 3 узла с одним `ifa_name`, разным `sa_family`); список т.к. число узлов заранее неизвестно; `freeifaddrs(head)` один раз, строки/адреса живут внутри аллокации (копировать значения, не указатели). **Полиморфизм `sockaddr`**: `ifa_addr`/`ifa_netmask` = `struct sockaddr *`; проверка `ifa_addr != NULL && sa_family == AF_INET` **до** каста (порядок в `&&` важен — NULL-guard первым). **Маска→префикс**: `(sockaddr_in*)ifa_netmask` → `.sin_addr.s_addr` → `__builtin_popcount` (ntohl не нужен — popcount инвариантен к порядку байт). **Модель маршрутизации** (узкое место, спросил прямо): Destination = целевая **сеть** правила, Gateway = next hop; `Gateway==0` → сеть локальная (прямая доставка), `Gateway!=0` → отдать роутеру (MAC гейтвея, не сервера); **default route** = `Destination 0.0.0.0/0`, совпадает со всем = выход наружу; ядро gateway не «выясняет» — приходит по DHCP (option 3) через rtnetlink. **Endianness** (узкое место): три вида IPv4 — network-order байты в `in_addr` / строка `inet_ntop` / host-order hex в `/proc`; `sscanf("%x")` даёт слово, которое на LE **уже** лежит как network-байты → `g.s_addr = gw` как есть, без ntohl (печать ядром и парсинг на одной машине гасят переворот). Правило: не «нужен ли ntohl», а «в каком виде данные СЕЙЧАС и что ждёт потребитель». Уроки `/check`: каст маски в `(in_addr*)` вместо `(sockaddr_in*)` → `s_addr` читает `sin_family`+`sin_port` (offset 0..3, а адрес на offset 4) → popcount(2)=1 = всегда `/1` (тихо неверно, не падение); `/%c` вместо `/%d` (символ кода 24); печать внутри цикла по строкам `/proc` → дубль `(no gateway)` + `lo` пропадает (решение: цикл находит default → флаг, печать один раз после по `ifa_name`); `%15s` в `sscanf` (overflow-guard); спам stderr на фильтре. Прогон в Docker (обе ветки). Слабое место: byte order всё ещё требует проговаривания по шагам, не автоматизм; мех. промахи держались лучше (файл меньше) — крупный баг был концептуальный (каст), не усталостный. → [[interface-enumeration]].
- **B-15** (интеграционное, первое в Фазе 3) — scanner v2 (`scanner-v2.c`, синтез B-10/B-11/B-12/B-13): scan диапазона + идентификация сервиса + **извлечение детали**. **Две фазы, разные модели I/O**: discovery (non-blocking `epoll`, окно слотов, машина B-10) отдаёт **список номеров** открытых портов; identification (blocking per-port, свежий `connect_timeout` + `run_probe`, машина B-13) коннектится заново. Trade-off «не переиспользовать сокет из фазы 1» соблюдён (+1 RTT ради чистого разделения; переиспользование вернуло бы проблему лимита fd). **Окно ≠ результат**: `probe_port slots[WINDOW]` (`fd`+`port`+`deadline`) vs `long *opened_ports` — разные структуры, разные счётчики (`i` слота vs `n_opened`). **Новое = извлечение детали** (ядро синтеза, B-11/B-12): развилка по `service` → `extract_http_server` (строка `Server:`, граница `\r\n`) / `extract_banner_version` (первая строка баннера до `\r\n`); обе **byte-exact по `res`**, пишут в переданный caller'ом out-буфер (не возвращают указатель внутрь recv-буфера → dangling; `out_size` = capacity, вход). Уроки `/check` (2 кластера): **(A) copy-paste из portscan.c** — окно `slots` внутри внешнего `while` (пересоздание, потеря сокетов); разбор событий вложен во **внутренний** `while` заполнения → зависание на хвосте (порты кончились, `active>0`, `epoll_wait` не зовётся) — разбор должен быть на уровне внешнего цикла; `opened_ports[c_opened]` vs `[i]` (OOB-read); сигнатура↔вызов рассинхрон при правке (правка сигнатуры = правка всех вызовов); `deadline` `int` вместо `long long`. **(B) byte-exact** — `strcmp(p," ")` на нетерминированном recv-буфере = OOB-read/UB (бежит до `'\0'` мимо `end`) → `*p==' '`; забытый `else break` → бесконечный цикл; `memcpy` из `buf` вместо `p`; инверсия skip-spaces (искал первый пробел вместо пропуска); `"Server: "`/7 рассинхрон паттерна и длины. Кириллическая `т` в `{` → сборка упала, тест шёл на старом бинарнике (заметно по несанированному выводу). E2E в Docker покрыл все ветки (HTTP-extract, SSH-баннер, closed, filtered, unknown). Слабое место (подтверждено): механические промахи при copy-paste рефакторинге, не непонимание. Осознанно отложено: санитизация недоверенных байт (`isprint` против terminal injection через ANSI-escape). → [[service-identification]].
- **B-14** — TLS handshake observational (Wireshark на живом `curl https://example.com` в Kali, decrypt через `SSLKEYLOGFILE`, **без кода**). Формат: `/theory` режим A (мини-лекция по 5 концепциям до наблюдения) → разбор захвата по 8 шагам → самопроверка Q1–Q6. **Три задачи handshake**: negotiation (версия/cipher suite/ECDHE-группа) + authentication (Certificate + CertificateVerify) + key agreement (ECDHE key_share) — всё до первого прикладного байта. **ECDHE**: общий секрет вычисляется независимо (аналогия смешивания красок), наблюдатель не «разделит» смесь — discrete log problem; Ephemeral → forward secrecy; `key_share` в ClientHello и ServerHello. **Асимметричная крипта** (сложное место сессии): пара ключей спаяна при рождении, sign приватным / verify публичным, успешный verify = proof of possession → отсекает MITM. **Certificate ≠ CertificateVerify**: первый копируется (публичный), второй доказывает владение приватным. **Chain of trust**: issuer[N]=subject[N+1], root в trust store не шлётся, self-signed никто не доверяет. **Record layer + AEAD**: всё в записях `[type][version][length]`, один record ≠ TCP-сегмент (reassembly), AEAD-tag ловит tampering → `bad_record_mac`; в 1.3 content type маскируется под `23`. **SNI** cleartext (выбор сертификата до шифра) → SNI-блокировка, обход через **ECH** (ключ из DNS HTTPS-record, только с шифрованным DNS). **ALPN** — выбор HTTP/2 vs 1.1 внутри handshake, 0 RTT. **1.2 vs 1.3**: 1.3 = 1 RTT, Certificate внутри шифра (в 1.2 cleartext). Version-ловушка: правда в `supported_versions`, поля Version врут `1.2` ради middlebox-совместимости (protocol ossification). Связал с [[service-fingerprint]] (443 client-first → `recv` виснет). Слабое место: **плотность TLS** — крипта + объём полей/заголовков разом. → [[tls]].
- **B-12** — DNS протокол вручную (`dnsquery.c`, A-record поверх UDP): собрал DNS-запрос по байтам, распарсил ответ, без `getaddrinfo`. **Wire format**: 12-байтовый заголовок (ID, Flags-битполе, QDCOUNT/ANCOUNT), QNAME как length-prefixed labels (точек нет), RR = NAME/TYPE/CLASS/TTL/RDLENGTH/RDATA. **Битовые поля**: извлечение маской+сдвигом (QR=`(flags>>15)&1`, RCODE=`flags&0x0F`), RD=`0x0100`. **Endianness**: сдвиги `>>8`/`&0xFF` работают над значением (переносимы); `htons` внутри = byte swap / no-op на BE, решается на компиляции. **Три гейта**: qr→rcode→ancount (порядок важен — `ancount==0` до `rcode` спрятал бы NXDOMAIN; NODATA ≠ NXDOMAIN). **Компрессия имён**: одна универсальная `skip_name` для labels и `0xC0`-указателя (SKIP-режим, за указателем не ходит → петля не страшна). Уроки `/check`: разыменование до проверки границы (OOB); UB на **образовании** заграничного указателя `p+rdlength` (не только разыменовании) → `rdlength > end - p`. Слабое место: битовые операции, endianness, квалификаторы указателей (const/знаковость). → [[dns]].
- **B-11** — HTTP basics (`httpget.c`, HTTP/1.1-клиент): HTTP = текстовый request/response поверх TCP, «протокол живёт в парсинге строк, не в сисколлах». **Framing** (граница сообщения в байтовом потоке) — 3 способа: `Content-Length` / `Transfer-Encoding: chunked` / `Connection: close`; трёхсторонняя развилка тела через `enum`. **Length-based парсинг** (`memmem`/`%.*s`/`strncasecmp`), без доверия к `\0` на бинарных данных. chunked-декодер: `strtol(...,16)` hex-размер, терминатор 0, проверки границ. Уроки (4 итерации `/check`): `&` на том, что уже указатель (`ai_addr`); последний заголовок — его `\r\n` внутри `sep`, `memmem`→NULL нужен guard; `int size` на hex переполняется в отрицательное → OOB (нужен `long` + `size > end - p`); неинициализированный out-param → OOB-read/утечка; `memchr(' ')` без NULL-проверки → SIGSEGV. Security-инсайт: скучный код границ/overflow = самый критичный. → [[http]].
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
- [[capabilities]] — Linux capabilities + raw sockets (B-17): дробление root на биты, `CAP_NET_RAW`, наборы permitted/effective/inheritable (effective проверяется в момент syscall, fd переживает drop), minimum capability surface, `setcap +ep` vs setuid-root, libcap-ng (буфер vs процесс, `clear` void → `apply` коммит), `_BOTH` требует `CAP_SETPCAP` → `_CAPS`

### Темы — Rust

- [[toolchain]] — rustup, rustc, cargo, Cargo.toml/lock, crate, edition, crates.io
- [[guessing-game-notes]] — открытые вопросы после главы 2 The Book (закрыто 1,2,3,5,6,7; открыто 4,8)
- [[types-control-flow]] — глава 3: immutable by default, shadowing, типы, statements vs expressions, ranges
- [[ownership]] — глава 4: move/copy, `&T`/`&mut T`, slices, dangling, реальные сообщения borrow checker'а, Drop/RAII (освобождение по владельцу)
- [[structs-methods]] — глава 5: struct (named/tuple/unit), `impl`, `&self`/`&mut self`/`self`, associated function vs method, `Self`, automatic referencing, `#[derive(Debug)]` + `{:?}`/`{:#?}`/`dbg!`
- [[enums-match]] — глава 6: enum с данными, `match` как выражение (exhaustive), `Option<T>` вместо null, `if let`, match ergonomics
- [[modules]] — глава 7: package ⊃ crate ⊃ module ⊃ item, пути `crate`/`super`/`self`, `pub`-видимость + `E0603`, `use` как ярлык (не `#include`), prelude
- [[collections]] — глава 8: `Vec`/`String`/`HashMap`, `entry` API (`or_insert_with`/`or_insert`), `*`-разыменование счётчика, `v[i]` vs `v.get`, итерация borrow vs move

- **B-13** — Service fingerprinting (`fingerprint.c`): идентификация сервиса по **ответу на пробу**, не по порту (порт лишь выбирает пробу). Ядро — **server-first vs client-first**: SSH/SMTP/FTP шлют баннер сами после connect (`recv` сразу), HTTP молчит до запроса (`send` payload → `recv`) — кодируется флагом `send_first` в `struct probe`. Статическая таблица `const probe` + `probes_for_port`. Пайплайн: `connect_timeout` (неблокирующий connect + select + `getsockopt(SO_ERROR)`, writable≠connected) → `run_probe` (**снять `O_NONBLOCK`** ради `SO_RCVTIMEO`, `send` в цикле-аккумуляторе при partial send, `recv`) → `memmem` по длине. Грабли `/check` (6 итераций): `strlen(NULL)` → SIGSEGV на server-first (payload==NULL, ловится только прогоном); неинициализированные аккумуляторы `sent`/`total` (рецидив класса); молчаливое усечение порта до валидации; fd leak на `continue` мимо `close`; `%s` по recv-буферу (нужен `%.*s`+`memmem`). Слабое место: **механические промахи на объёме error-handling/ветвей** (не непонимание — усталость). → [[service-fingerprint]].

### Темы — Networking

- [[tcp-sockets]] — TCP-сокеты, socket/bind/listen/accept/connect, partial read, EPIPE, SIGPIPE защита
- [[nonblocking-poll]] — blocking vs non-blocking сокеты + poll: матрица 2×2, три роли (режим/poll/timeout/recv), EINPROGRESS connect, SO_ERROR (прицельно под слабое место B-6)
- [[event-loop-epoll]] — event loop model, epoll, non-blocking I/O, readiness ≠ correctness
- [[connect-timeout-scan]] — connect-with-timeout, примитив connect-scan (B-6+B-7): три исхода open/closed/filtered, SO_ERROR = вердикт, синхронный vs async refused, Docker Desktop NAT врёт
- [[dns]] — DNS wire format вручную (B-12): заголовок/флаги-битполе, QNAME labels, RR, name compression (`0xC0`), три гейта qr/rcode/ancount (NODATA vs NXDOMAIN), endianness, UB образования заграничного указателя
- [[service-fingerprint]] — service fingerprinting (B-13): решает ответ не порт, server/client-first (`send_first`), таблица `const probe`, `connect_timeout`+`run_probe`, `SO_RCVTIMEO` требует снять `O_NONBLOCK`, `memmem` по длине, `strlen(NULL)`→SIGSEGV, partial send
- [[tls]] — TLS 1.3 handshake observational (B-14): три задачи (negotiation/auth/key agreement), ECDHE key_share + forward secrecy, асимметричная крипта (sign/verify, proof of possession), Certificate vs CertificateVerify, chain of trust (issuer=subject), record layer + AEAD, SNI/ECH, ALPN, version-ловушка (supported_versions), Wireshark decrypt через SSLKEYLOGFILE
- [[interface-enumeration]] — interface enumeration (B-16, первое Фазы 4): `getifaddrs` (связный список, узел=адрес не интерфейс, полиморфизм `sockaddr`, `sa_family` до каста), маска→префикс через popcount (ntohl не нужен), модель маршрутизации (`/proc/net/route`, Destination/Gateway/default route, next hop = MAC роутера), endianness (три вида IPv4, `/proc`-hex = host-order слово = network-байты на LE)
- [[service-identification]] — scanner v2 (B-15, интеграционное): две фазы (discovery non-blocking epoll / identification blocking per-port), окно ≠ результат, извлечение детали (extract_http_server `Server:` / extract_banner_version баннер), byte-exact по `res`, запись в переданный out-буфер (не dangling), санитизация untrusted (isprint против terminal injection), copy-paste рефакторинг = пересборка инвариантов
- [[udp-sockets]] — UDP sockets: recvfrom/sendto, connectionless, датаграммные границы (усечение), потеря пакета = SO_RCVTIMEO/EAGAIN, %s по recvfrom-буферу = OOB-read
- [[port-scan-scaling]] — масштабирование connect-scan: bounded concurrency, окно проб, fd→slot через data.u32, relative epoll timeout, liveness-условие цикла (порты ИЛИ активные пробы), счётчик in-flight
- [[nagle-tcp-nodelay]] — алгоритм Нагла, TCP_NODELAY, когда выключать
- [[http]] — HTTP/1.1 basics: request/response, framing (Content-Length/chunked/close), CRLF, chunked-декодирование, length-based парсинг, security-границы (§4)
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
- [[pointer-arithmetic-ub]] — UB образования заграничного указателя (не только разыменования) → `len > end-p`; квалификаторы `const`/знаковость `char*` vs `uint8_t*`

### Темы — Reading (разборы чужого кода)

- [[musl-popen]] — musl libc popen/pclose/_Fork, fd inheritance, FD_CLOEXEC
- [[redis-anet]] — Redis обёртка над sockets API: симметрия connect/server, гигиена сокета, паттерн goto error
