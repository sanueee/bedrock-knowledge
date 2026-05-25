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
| Rust | Старт. Toolchain (rustc 1.94.0, cargo). The Book гл. 1–5 прочитаны. Понимаю: immutable by default, `mut`, shadowing, типы (`i32`/`u32`/`usize`/`bool`/`char` Unicode, overflow panic в debug), statements vs expressions, ranges (`..` / `..=`), `if`/`loop`/`for in collection`. Ownership: move vs Copy, `&T` shared XOR `&mut T` exclusive, `String` (owned heap) vs `&str` (slice view), slices без аллокации, deref coercion, dangling = compile error. Видел реальные сообщения borrow checker'а (E0499/E0502/E0382/E0106/E0515). Structs: named-field/tuple/unit-like, field init shorthand, `impl` блок, три формы receiver (`&self`/`&mut self`/`self`), associated function vs method (конструктор `new` — конвенция, не языковая фича), `Self` как алиас типа, automatic referencing для receiver (но не для аргументов), `#[derive(Debug)]` + `{:?}`/`{:#?}`/`dbg!`. Написал `word_tools` (slices) и `rectangles` (struct + методы). Открыто: enums + `match` exhaustive (гл. 6), `Result`/`?` (гл. 9), traits (гл. 10), lifetimes (гл. 10). |
| Linux | Комфортно в терминале. /proc — знаю предметно (parsing, fd, маршрутизация запросов через `/proc/[pid]/`). Понимаю kernel/user boundary, syscalls, fd kernel model + refcount, virtual memory + copy-on-write, async-signal-safety, EINTR-семантику. Internals глубже (планировщик, VFS, namespaces) — пока поверхностно. |
| Git | Базово (add/commit/push/log/diff), формирую привычку коммитить по смыслу. Ребейзы, rerere, bisect, worktrees — пока не трогал. |
| Сети | TCP/IP концептуально (handshake, TIME_WAIT, RST, FIN, partial read/write на stream). /proc/net/tcp — парсил. Сокеты — написал echo server+client (B1). Захват пакетов — написал sniffer на libpcap (B2): BPF фильтр, парсинг Ethernet/IP/TCP, флаги через bitwise AND, payload_len = ntohs(ip_len) - ip_hl*4 - th_off*4. |
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
│       └── llm-sec/     # LLM-security (этап 3)
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
| Я пишу "мок-собес" / "проверь меня" / "вопросы по пройденному", или give-task видит счётчик ≥ 8 | `.claude/skills/mock-interview/skill.md` |
| Я пишу "дай почитать код", "хочу разобрать чужой код", или give-task видит счётчик reading ≥ 4, или завершён блок | `.claude/skills/reading-code/skill.md` |
| Я пишу "дай ctf", "хочу picoctf", "ctf-сессия", или give-task видит счётчик ctf ≥ 5 | `.claude/skills/ctf/skill.md` |
| Я пишу "на этом все" / "заканчиваем", или Claude понял что тема усвоена | `.claude/skills/session-debrief/skill.md` |

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

### Английский — рабочий технический словарь

Уровень пользователя: **B2** (читает и смотрит свободно с субтитрами; говорить и слушать на слух — слабее).
Цель — выстроить английский технический словарь через регулярное использование.

- Ключевые технические термины давать **на английском с русским переводом в скобках при первом упоминании**:
  "состояние гонки (race condition)", "захват блокировки (lock acquisition)".
- При повторных упоминаниях в той же сессии — можно только английский термин.
- Названия функций, сисколлов, флагов, типов — **только английский**, никогда не транслитерировать (`fork()`, `EAGAIN`, `O_NONBLOCK`).
- Идиомы у которых есть устоявшийся английский термин (ownership, lifetime, syscall, kernel space, race condition) — давать на английском, не калькировать.
- В vault: каждая запись в `topics/` имеет раздел `## Ключевые термины (English)` — глоссарий темы. Это словарь для будущих собеседований и чтения мануалов.
