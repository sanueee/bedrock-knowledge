---
обновлено: 2026-07-12 (закрыт B-13 — fingerprint.c, атомарное, Фаза 3: service fingerprinting. Перед кодом — /theory режим A (мини-лекция по теории задания ДО кода: server/client-first, connect-таймаут, SO_RCVTIMEO+recv-семантика, memmem vs strstr). Библиотека проб SSH/HTTP/SMTP/FTP: статическая таблица `const probe` {send_first/payload/pattern/service} → probes_for_port(port) выбирает пробу → connect_timeout (неблокирующий connect + select + getsockopt SO_ERROR) → run_probe (снять O_NONBLOCK ради SO_RCVTIMEO, send в цикле-аккумуляторе, recv) → memmem по длине в main. Классификация по СОДЕРЖИМОМУ ответа, порт лишь выбирает пробу. /check 6 итераций: strlen(NULL)→SIGSEGV на server-first (payload==NULL, поймал только ПРОГОН — не компилятор/чтение); неинициализированные аккумуляторы sent/total (рецидив класса дважды); молчаливое усечение порта до валидации (chtol→long); fd leak на continue мимо close; %s по recv→нужен %.*s+memmem; timeout_ms=3 (мс!) флейк. -Wall -Wextra чисто, оттестирован end-to-end (SSH scanme, HTTP Apache, closed, unknown). Слабое место сессии (сам сформулировал): МЕХАНИЧЕСКИЕ промахи на объёме error-handling/ветвей — не непонимание, а усталость от количества исключений; concept-стыки усвоены на /theory и в коде не ломались. Позиция 13/26)
предыдущее обновление: 2026-07-07 (закрыт B-12 — dnsquery.c, атомарное, Фаза 3: DNS протокол вручную. Перед кодом — /theory-прогон устройства пакета в 4 слоя (ecosystem/UDP → заголовок → question/labels → answer/компрессия), затем конспект topics/networking/dns.md. Собрал DNS-запрос по байтам, распарсил ответ без getaddrinfo. Wire format: 12-байтовый заголовок (Flags-битполе), QNAME length-prefixed labels, RR NAME/TYPE/CLASS/TTL/RDLENGTH/RDATA. Одна универсальная skip_name для labels и 0xC0-указателя (SKIP-режим). Три гейта qr→rcode→ancount (NODATA≠NXDOMAIN). Уроки /check: разыменование до проверки границы (OOB); UB на ОБРАЗОВАНИИ заграничного указателя p+rdlength, не только разыменовании → rdlength>end-p. Работает end-to-end (example.com/dns.google/NXDOMAIN), -Wall -Wextra чисто. Слабые места сессии: битовые операции (установка/извлечение флага), endianness, квалификаторы указателей (const/знаковость мылят глаз). ВАЖНО про процесс: я (Claude) дважды дописал код за пользователя (build_query по явной просьбе — ок; parse_answers — не следовало, всплыло раздражение «зачем дописал»); при просьбе «поправь всё» не авторить пустые security-критичные тела, а вести через вопросы. Позиция 12/26)
---

# Roadmap — диспетчер проекта

**Текущая позиция и навигация.** Самый часто читаемый файл — держится лёгким: только суть, следующий шаг, слабые места. Детали по необходимости — в соседних файлах:

- **Закрытые/пройденные темы по блокам + индекс тем (оценка знаний, пересмотр стратегии):** [archive.md](archive.md)
- **Журнал выполненного (задачи + код, reading/theory/mock/ctf):** [journal.md](journal.md)
- **План и принципы (будущее):** [learning-strategy.md](../strategy/learning-strategy.md)
- **Контекст о пользователе + реестр скиллов:** [CLAUDE.md](../../.claude/CLAUDE.md)

Код+задания вместе: `sections/<domain>/<name>/` (domain: `ds`/`linux`/`networking`/`rust`/`crypto`) | Теория: `knowledge-base/topics/` | leetcode: `sections/leetcode/`

---

## Текущая позиция

> Обновляется после каждой завершённой сессии через `session-debrief`. **Это primary source** для всех skills (give-task, mock-interview, reading-code, ctf).

**Снимок уровня по стеку** (детали усвоенного — в [archive.md](archive.md)):
- **C** — джун, уверенно: системка (POSIX/proc, signals, IPC, pthreads) + сеть (сокеты, libpcap).
- **Структуры данных** — теория курса + реализации на C (массивы/списки/стек/очередь/дека, деревья BST/AVL/splay/B, хеши, графы, сортировки). Слабые: AVL LR/RL, вывод констант сложности. Детали — [[index]] + [archive.md](archive.md).
- **Rust** — старт: The Book гл. 1–8 (ownership, structs, enums/match, modules, collections). Дальше: `Result`/`?`, traits, lifetimes. Слабые места: ownership-терминология, уровни package/crate/module.
- **Linux** — комфортно в терминале; /proc, fd-модель, virtual memory + CoW, EINTR.
- **Git** — базово (add/commit/push/log/diff); rebase/bisect/worktrees — не трогал.
- **Сети** — TCP/IP концептуально; написаны echo, sniffer, DNS-resolver, banner-grab, UDP echo, port-scan (connect-scan диапазона, bounded concurrency), HTTP/1.1-клиент (framing, chunked-декодер), DNS-протокол вручную (wire format, name compression, битовые поля флагов), **service fingerprinting (server/client-first пробы, `SO_RCVTIMEO` требует снять `O_NONBLOCK`, `memmem`-матчинг по содержимому)**. Слабые места: адресные структуры (`sockaddr_in` init — `memset`+`sin_family` забывается); **механика подсчёта байт** — трактовать ровно `n`, `%s` по неполной строке = OOB-read (только `%.*s`). **Парсинг сетевых данных**: guard'ы на `memmem`/`memchr`/границы, overflow целочисленных длин из сети — горячая зона (см. [[feedback-tedious-security-code]]). **МЕХАНИЧЕСКИЕ промахи на объёме error-handling/ветвей** (B-13): неинициализированные аккумуляторы, `strlen(NULL)` на опциональном поле, fd leak на `continue` мимо `close` — не от непонимания, а от усталости на количестве исключений; concept-стыки при этом усваиваются нормально. Практика: код с многими ветвями гонять медленнее, чек-лист «каждый выход закрывает ресурс», прогон edge-веток обязателен (баг `strlen(NULL)` поймал только тест, не чтение). **Битовые операции** (установка/извлечение полей маской+сдвигом), **endianness** (сдвиги над значением vs байты в памяти, htons=byteswap), **квалификаторы указателей** (`const`/знаковость `char*` vs `uint8_t*` — VS Code подсвечивает, mental model сырая) — всплыло на B-12, разобрать глубже. **UB арифметики указателей**: образование заграничного указателя (`p+len`) = UB до разыменования → сравнивать остаток `len > end-p` (см. [[pointer-arithmetic-ub]]).
- **Навигация по объёмному коду** (не сеть, общий навык; всплыло на B-10 `portscan.c` ~260 строк) — логика ясна, но целостная карта крупного интеграционного файла (какой цикл что делает, где освобождается слот, какой `active--` парен `++`) собирается тяжело. Для `/check`/ревью больших файлов с вложенными циклами и общим массивом состояния — до разбора багов дать карту структуры + таблицу инвариантов ресурса. См. [[feedback-nested-loop-control-flow]].
- **Английский** — B2.
- **Инструментальные:** Python (школьный базис) — LLM-обвязка, SAST, CTF; Go (0) — eBPF user-space, DevSecOps-тулинг. Учим по необходимости, без отдельных учебных блоков.

**Блоки:**
- **A — Linux internals + Data Structures (C)** — ✅ закрыт (структура пройденного → [archive.md](archive.md)).
- **B — Сетевой стек (C)** — 🔵 активный, 13/26.
- **C — Rust (параллельный трек)** — 🔵 активный, 5/?.

### Активный блок B — Сетевой стек (13/26)

**Структура:** 26 заданий (20 атомарных + 5 интеграционных + 1 магнум опус), интеграционные на 5/10/15/20/25, опус — 26. Карта тем зафиксирована 2026-05-25 (стратегия → "### Блок B"). Базовая структура растягивается под глубину темы (скилл `plan-block`).

**Магнум опус (B-26):** network scanner — ARP discovery + ICMP ping sweep + TCP connect scan + TCP SYN scan + service detection. Отдельная репа на GitHub.

**Позиция:** 13/26 (B-1…B-13 — детали в [archive.md](archive.md), таблица заданий в [journal.md](journal.md)). Последний task — task-b13 (`fingerprint`, service fingerprinting, 2026-07-12).

**Следующий шаг — B-14 (позиция 14, АТОМАРНОЕ, Фаза 3), TLS handshake observational:** Wireshark на HTTPS-сессии, разбор ClientHello/ServerHello/Certificate/Finished, SNI extension, ALPN. **Без кода.** Место: `topics/networking/tls.md`. Закрывает чекпойнт этапа 1 «понимаешь что происходит при connect() на уровне ядра» — расширяет на L7. Детали — стратегия → «### Блок B» → B-14. **После B-14 — B-15 (ИНТЕГРАЦИОННОЕ, позиция 15): scanner v2** — scan диапазона (B-10) + identification сервиса каждого открытого порта пробами из B-13. Синтез B-11/B-12/B-13 поверх B-10.

### Параллельный блок C — Rust (5/?)

**Структура:** Ведётся линейно по главам The Book — одна глава = одно (обычно атомарное) задание `C-N`. Под структуру «4 атомарных + интеграционное + опус» не переводится: правило «каждое 5-е интеграционное» к Rust-треку не применяется (скилл `plan-block`).

**Позиция:** C-5 завершён (главы 7–8 — modules + collections, `scan_aggregator`). Трек **отложен** — приоритет отдан блоку B (реш. 2026-07-02, [[feedback-prioritize-block-b]]). **Следующий шаг при возврате — C-6 (глава 9):** error handling — `Result`/`?` (закрывает вопрос 4 из [[guessing-game-notes]]). Файл задания `task-c06-rust-ch9-error-handling.md` **уже создан** (2026-07-02, ждёт). Дальше по карте: гл.10 (traits/generics/lifetimes), гл.11 (тесты), гл.12 (minigrep). Карта блока — стратегия → "### Блок C".

### Невыполненные закрепляющие задания

Пусто

### Параллельные блоки/треки доступны сейчас

- **Главный трек — B-14 (атомарное, TLS handshake observational: Wireshark на HTTPS, ClientHello/ServerHello/Certificate/Finished, SNI, ALPN, БЕЗ кода → topics/networking/tls.md). Рекомендуемый следующий шаг:** пользователь 2026-07-02 выбрал **приоритет блока B над ритмом 3:1** — хочет закрыть сетевой блок раньше (см. [[feedback-prioritize-block-b]]). По умолчанию рекомендовать B, не переключать на C по счётчику, пока пользователь не скажет иначе. Прим.: B-14 — observational без кода; после него B-15 интеграционное (scanner v2).
- **Блок C — Rust** (параллельный, отложен по решению пользователя). Следующий шаг при возврате — C-6 (глава 9, error handling); файл `task-c06-rust-ch9-error-handling.md` уже создан и ждёт.
- **Мок-собес / reading-code / CTF** — управляются скиллами (по запросу пользователя или решению Claude), не счётчиками. См. таблицу триггеров в [CLAUDE.md](../../.claude/CLAUDE.md).

При запросе задания `give-task` обязан учитывать что пользователь может выбрать главный трек **или** параллельный. Если параллельные блоки доступны — спросить какой трек.
