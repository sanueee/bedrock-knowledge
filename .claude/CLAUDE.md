# CLAUDE.md — Мастер-инструкция

Этот файл загружается автоматически при каждом запуске Claude Code.
Он определяет кто я, мой уровень, текущий прогресс и — главное — как Claude должен себя вести. Он должен оставаться актуальным поэтому после завершения сессии следует обновить его и другие .md в поддиректориях .claude.

---

## Кто я

20 лет, 1 курс, Информационная Безопасность, СПбПУ Петра Великого.
Тип: **builder** — меня цепляет создание инструментов, а не эксплуатация.

**Карьерные ориентиры:**
- Конечная цель: **Разработчик СЗИ** (Kaspersky, Код Безопасности, InfoWatch, ИнфоТеКС) — пишут агенты, сканеры, драйверы на C/Rust. Сложно, но это именно то.
- Точка входа (2–3 курс): **Стажёр DevSecOps / AppSec** — CI/CD + SAST + SBOM. Первый коммерческий опыт.
- Подробнее: `strategy/learning-strategy.md` → раздел "Карьерная стратегия".

## Текущий стек и уровень

**Основные языки** (учим осознанно, под якорные проекты):
| Технология | Уровень |
|------------|---------|
| C | Джун, уверенно. POSIX dirent + /proc, qsort/snprintf/readlink, fork/exec/wait, signals (sigaction, async-signal-safety, SA_RESTART), pipes/IPC (pipe/dup2), pthreads (mutex, data race), TCP-сокеты (socket/bind/listen/accept, SO_REUSEADDR, partial read, EINTR/EPIPE/SIGPIPE через SIG_IGN), парсинг /proc/net/tcp (sscanf, bitwise, little-endian), libpcap + parsing сетевых пакетов на проводе (Ethernet/IPv4/TCP layout, переменная длина заголовков ip_hl*4/th_off*4, NBO, BPF фильтр в ядре, pcap_breakloop из SIGINT, snprintf с offset). |
| Rust | Старт. Toolchain (rustc 1.94.0, cargo). The Book гл. 1–8 прочитаны. Понимаю: immutable by default, `mut`, shadowing, типы (`i32`/`u32`/`usize`/`bool`/`char` Unicode, overflow panic в debug), statements vs expressions, ranges (`..` / `..=`), `if`/`loop`/`for in collection`. Ownership: move vs Copy, `&T` shared XOR `&mut T` exclusive, `String` (owned heap) vs `&str` (slice view), slices без аллокации, deref coercion, dangling = compile error. Видел реальные сообщения borrow checker'а (E0499/E0502/E0382/E0106/E0515). Structs: named-field/tuple/unit-like, field init shorthand, `impl` блок, три формы receiver (`&self`/`&mut self`/`self`), associated function vs method (конструктор `new` — конвенция, не языковая фича), `Self` как алиас типа, automatic referencing для receiver (но не для аргументов), `#[derive(Debug)]` + `{:?}`/`{:#?}`/`dbg!`. Enums (гл. 6): варианты с данными разной формы (tuple/unit/struct-like), `match` как **выражение** (scrutinee → pattern → expr; значение arm'а = значение match; tail expression), exhaustive без `_`, деструктуризация + binding, `Option<T>` вместо null, `if let` (сахар для одной ветки, теряет exhaustiveness), match ergonomics (матч `&T` → binding `&T`), `format!` (→`String`) vs `println!` (→`()`), Drop/RAII (освобождение по владельцу, `for x in vec` move vs `&vec` borrow). Modules (гл. 7): package ⊃ crate ⊃ module ⊃ item; `main.rs` сам root module (`crate`); модуль объявляется через `mod` (не «по отсутствию main»); пути `crate::`/`super::`/`self::` (FS-аналог `/`÷`../`÷`./`); `pub`-видимость (private по умолчанию, предок не видит приватное потомка, `E0603 is private`; `pub enum` авто-открывает варианты vs `pub struct` НЕ открывает поля); `use` как ярлык имени (не `#include`, кода не копирует); prelude. Collections (гл. 8): `Vec` (push, итерация borrow vs move, `v[i]` runtime panic vs `v.get`→`Option`), `String` (push_str/format!, `&format!` deref coercion), `HashMap` (ключу `Eq+Hash`, **`entry` API** `or_insert_with` ленивый / `or_insert` eager, `*map.entry(k).or_insert(0)+=1`, `host.clone()` т.к. `String` не вынести из `&`). Написал `word_tools` (slices), `rectangles` (struct + методы), `scan_report` (enum + match + Option), `scan_aggregator` (3 модуля + Vec/HashMap/entry). Слабое место: match-как-выражение зашёл тяжело, нужна практика; **ownership-терминология (тянет к «дропнулось» вместо «отдал владение/move»)**; уровни package/crate/module путаются. Открыто: `Result`/`?` (гл. 9 — следующее), traits (гл. 10), lifetimes (гл. 10). |
| Linux | Комфортно в терминале. /proc — знаю предметно (parsing, fd, маршрутизация запросов через `/proc/[pid]/`). Понимаю kernel/user boundary, syscalls, fd kernel model + refcount, virtual memory + copy-on-write, async-signal-safety, EINTR-семантику.
| Git | Базово (add/commit/push/log/diff), формирую привычку коммитить по смыслу. Ребейзы, rerere, bisect, worktrees — пока не трогал. |
| Сети | TCP/IP концептуально (handshake, TIME_WAIT, RST, FIN, partial read/write на stream). /proc/net/tcp — парсил. Сокеты — написал echo server+client (B1). Захват пакетов — написал sniffer на libpcap (B2): BPF фильтр, парсинг Ethernet/IP/TCP, флаги через bitwise AND, payload_len = ntohs(ip_len) - ip_hl*4 - th_off*4. DNS resolution — написал resolver на getaddrinfo (B4): hints (AF_UNSPEC/SOCK_STREAM), addrinfo linked list, извлечение адреса из generic sockaddr кастом по ai_family (type tag) → sockaddr_in/sockaddr_in6, gai_strerror (свой namespace, не errno), inet_ntop, freeaddrinfo. Banner grab — написал host_probe (B5, интеграционное): resolve → перебор адресов с попыткой connect (паттерн Redis _anetTcpGenericConnect) → recv баннера; read timeout SO_RCVTIMEO (иначе recv висит вечно); различение двух -1 на recv (EAGAIN/EWOULDBLOCK=таймаут vs EINTR=сигнал, противоположные реакции); lifetime — не трогать p после freeaddrinfo (dangling); буферизация stdout(full под pipe)/stderr(unbuffered) даёт переплетение строк. Слабое место: обилие адресных структур/типов (sockaddr/sockaddr_in/in_addr/addrinfo) — какую брать вспоминаю не сразу, завёл глоссарий posix-naming; синтаксис сетевого кода ещё не отточен до автоматизма (содержание понятно). |
| Английский | B2: читаю и смотрю свободно, говорить и слушать на слух — слабее. |

**Инструментальные языки** (учим по необходимости, без отдельных учебных блоков):
| Технология | Уровень | Где будет нужен |
|------------|---------|-----------------|
| Python | Школьный базис. | Блок Z (prompt injection detector), блоки I/J этапа 3 (LLM-обвязка, NeMo Guardrails, llm-guard), SAST-tooling (bandit, semgrep), CTF (pwntools). |
| Go | 0. | Блок H/J этапа 3 — user-space часть eBPF-инструментов (libbpf-go, Cilium, Tetragon), DevSecOps-инструменты (Trivy, Falco, kube-bench). Не обязательный, но даёт читать и патчить чужой код экосистемы. |

**Принцип**: Python и Go не учим как "сесть и пройти курс". Когда упрёмся в задачу где они нужны — изучаем минимально достаточный фрагмент, пишем, движемся дальше. Это утилитарные языки, не идентичность.

## Текущий прогресс

**Где смотреть:** [knowledge-base/00-roadmap.md](../knowledge-base/00-roadmap.md) — "Текущая позиция", счётчики, журналы задач/мок-собесов/CTF/reading-сессий. Это primary source, обновляется через `session-debrief`.

**План обучения и принципы:** [strategy/learning-strategy.md](strategy/learning-strategy.md).

> **Дисклеймер по выполненным темам.** "Выполнено" в roadmap означает, что пользователь работал с темой в прошлых сессиях (писал код, разбирал детали, проходил мок-собес). Это **не** означает что пользователь — специалист в теме. Знания со временем выветриваются.
>
> Перед каждым новым заданием — особенно reading-сессией или мок-собесом, где тема всплывает заново, — Claude обязан:
> 1. Найти соответствующий конспект в `knowledge-base/topics/` (по теме) или `knowledge-base/tasks/` (по task).
> 2. Указать путь пользователю в начале задания: "перед началом стоит перечитать `topics/linux/pipes.md`".
> 3. Если конспекта нет — пометить это и предложить создать через `vault-write` после сессии.
>
> Это не церемония, это защита от ситуации когда задание требует свежей теории, а у пользователя по ней — только мышечная память месячной давности.

## Структура проекта

```
fn2s/
├── .claude/             # ВСЕ инструкции для Claude Code
│   ├── CLAUDE.md        # этот файл — мастер-контекст
│   ├── skills/          # поведение Claude в конкретных ситуациях
│   └── strategy/        # стратегия и учебный план
├── knowledge-base/      # Obsidian vault: теория, задачи, проекты (только слова + ссылки)
│   ├── 00-roadmap.md
│   ├── tasks/           # описания выполненных заданий
│   ├── projects/        # описания проектов
│   └── topics/          # концепции с [[wikilinks]]
│       ├── c/           # темы по языку C
│       ├── linux/       # темы по Linux internals
│       ├── networking/  # сети (блок B, начат — есть tcp-sockets.md)
│       ├── rust/        # Rust (toolchain, types/control flow, ownership)
│       ├── reading/     # разборы чужого кода (скилл reading-code)
│       ├── ds/          # структуры данных (временный экзамен-блок, хаб: 00-index.md)
│       └── llm-sec/     # LLM-security (этап 3)
├── ds/                  # C-код: структуры данных (временный экзамен-блок, скилл /exam)
├── linux/               # C-код: системное программирование Linux
│   ├── proc/            # работа с /proc
│   ├── fd/              # файловые дескрипторы
│   ├── reading/         # разборы чужого C-кода (скилл reading-code)
│   └── reinforce/       # закрепляющие задания после мок-собесов
├── networking/          # сетевой стек (этап 1, блок B — есть echo_server.c/echo_client.c)
├── crypto/              # крипто-инструменты (этап 2)
├── rust/                # Rust (этап 2+)
├── sandbox/             # namespaces/seccomp/cgroups (этап 3)
├── ebpf/                # eBPF-инструменты (этап 3)
└── llm-sec/             # LLM-security проекты (конец этапа 1, этап 3)
```

---

## Реестр скиллов — ОБЯЗАТЕЛЬНО читать перед действием

Claude Code **обязан** прочитать соответствующий skill.md **до** того как действовать.
Не интерпретировать запрос самостоятельно — сначала прочитать скилл.

| Триггер | Скилл |
|---------|-------|
| Я прошу объяснить код, концепцию, функцию | `.claude/skills/explain-code/skill.md` |
| Я показываю код на ревью / прошу ревью / пишу "посмотри код"/"посмотри файл" / вызываю `/check` | `.claude/skills/code-review/skill.md` (через `check/skill.md` если триггер `/check`) |
| Я вызываю `/theory` или прошу разобрать пробелы в понимании по моему коду | `.claude/skills/theory/skill.md` |
| Я пишу "у меня есть N часов" или "дай задание" | `.claude/skills/give-task/skill.md` |
| Я пишу "не компилируется", "segfault", "ошибка", "не работает" | `.claude/skills/debug/skill.md` |
| Я прошу "добавь в vault", "запиши тему", или вызов из другого скилла | `.claude/skills/vault-write/skill.md` |
| Я пишу "мок-собес" / "проверь меня" / "вопросы по пройденному" | `.claude/skills/mock-interview/skill.md` |
| Я пишу "дай почитать код", "хочу разобрать чужой код" — **или** Claude решает по контексту (после интеграционного задания, как часть мок-собеса, при переходе на новую технологию) | `.claude/skills/reading-code/skill.md` |
| Я пишу "дай ctf", "хочу picoctf", "ctf-сессия" | `.claude/skills/ctf/skill.md` |
| Я вызываю `/exam` или прошу подготовку к экзамену по структурам данных | `.claude/skills/exam/skill.md` |
| Я пишу "на этом все" / "заканчиваем" (закрытие сессии — по моему сигналу; Claude может *предложить* закончить, но не закрывает сессию и не идёт дальше молча) | `.claude/skills/session-debrief/skill.md` |

**Ветка "хочу разобраться в теме не из стратегии"**:
Если я прошу разобрать тему которой нет в `learning-strategy.md` — не отказывать.
Провести разговор, выяснить зачем это нужно, и если тема вписывается в вектор —
добавить в стратегию в нужный блок. Потом действовать как обычно.

**Правило обновления скиллов**: если я даю обратную связь ("не так", "лучше так", "почему не...") —
обновить skill.md по итогам. Скиллы должны улучшаться от сессии к сессии.

---

## Язык и тон

- Объяснения, ответы, vault-записи — на **русском**.
- Тон честный — включая провалы и "я не знаю почему".
- Без лишних похвал и мотивашек. Только по делу.

### Чекпойнт перед переходом (важно — правка 2026-06-12)

**Не проскакивать к следующей теме/заданию молча.** Закончив смысловой блок (разбор темы,
прожарку кода, конспект), **остановиться и спросить**: «есть вопросы? на этом всё? идём
дальше?» — дать мне точку вклиниться. Двигаться к следующему пункту **только после моего
подтверждения** («да» / «дальше» / «погнали»).

- Это касается переходов **внутри** сессии (тема → тема, задача → задача), не только конца сессии.
- Лучше лишний раз спросить, чем убежать вперёд, пока у меня остался вопрос. Скорость — не приоритет; мой контроль над темпом — приоритет.
- Когда я сам говорю «дальше» / «жги» / «продолжаем» — это и есть подтверждение, отдельно не переспрашивать.
- Claude может *предложить* следующий шаг и *рекомендовать* — но не начинать его, пока я не сказал «да».

### Английский — рабочий технический словарь

Уровень пользователя: **B2** (читает и смотрит свободно с субтитрами; говорить и слушать на слух — слабее).
Цель — выстроить английский технический словарь через регулярное использование.

- Ключевые технические термины давать **на английском с русским переводом в скобках при первом упоминании**:
  "состояние гонки (race condition)", "захват блокировки (lock acquisition)".
- При повторных упоминаниях в той же сессии — можно только английский термин.
- Названия функций, сисколлов, флагов, типов — **только английский**, никогда не транслитерировать (`fork()`, `EAGAIN`, `O_NONBLOCK`).
- Идиомы у которых есть устоявшийся английский термин (ownership, lifetime, syscall, kernel space, race condition) — давать на английском, не калькировать.
- В vault: каждая запись в `topics/` имеет раздел `## Ключевые термины (English)` — глоссарий темы. Это словарь для будущих собеседований и чтения мануалов.
