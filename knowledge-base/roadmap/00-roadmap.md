---
обновлено: 2026-06-19 (DS-блок (структуры данных, экзамен 1 курса) свёрнут в блок A как завершённый: «Блок A — Linux internals + Data Structures (C)»; структура пройденного DS-материала (модули 0–8 + сквозные темы) записана в archive.md. Прошлая запись: закрыт B-7 — epoll_echo.c, многоклиентский echo на event loop. Ревью /check → 5 фиксов: SIGINT-выход через sigaction без SA_RESTART, EPOLLOUT-backlog (per-client conn_t + flush_out/arm), EINTR на recv, fd-leak, EPOLL_CTL_DEL с NULL. Прогон в Docker gcc-контейнере (epoll Linux-only): сборка чистая, 3 одновременных клиента — независимый echo. По словам — логика event-loop-сервера нова целиком + большой пласт нового синтаксиса. Позиция 7/26)
---

# Roadmap — диспетчер проекта

**Текущая позиция и навигация.** Самый часто читаемый файл — держится лёгким: только суть, следующий шаг, слабые места. Детали по необходимости — в соседних файлах:

- **Закрытые/пройденные темы по блокам + индекс тем (оценка знаний, пересмотр стратегии):** [archive.md](archive.md)
- **Журнал выполненного (задачи + код, reading/theory/mock/ctf):** [journal.md](journal.md)
- **План и принципы (будущее):** [learning-strategy.md](../../.claude/strategy/learning-strategy.md)
- **Контекст о пользователе + реестр скиллов:** [CLAUDE.md](../../.claude/CLAUDE.md)

Код: `linux/`, `networking/`, ... | Теория: `topics/` | Задания: `tasks/`

---

## Текущая позиция

> Обновляется после каждой завершённой сессии через `session-debrief`. **Это primary source** для всех skills (give-task, mock-interview, reading-code, ctf).

**Снимок уровня по стеку** (детали усвоенного — в [archive.md](archive.md)):
- **C** — джун, уверенно: системка (POSIX/proc, signals, IPC, pthreads) + сеть (сокеты, libpcap).
- **Структуры данных** — теория курса + реализации на C (массивы/списки/стек/очередь/дека, деревья BST/AVL/splay/B, хеши, графы, сортировки). Слабые: AVL LR/RL, вывод констант сложности. Детали — [[00-index]] + [archive.md](archive.md).
- **Rust** — старт: The Book гл. 1–8 (ownership, structs, enums/match, modules, collections). Дальше: `Result`/`?`, traits, lifetimes. Слабые места: ownership-терминология, уровни package/crate/module.
- **Linux** — комфортно в терминале; /proc, fd-модель, virtual memory + CoW, EINTR.
- **Git** — базово (add/commit/push/log/diff); rebase/bisect/worktrees — не трогал.
- **Сети** — TCP/IP концептуально; написаны echo, sniffer, DNS-resolver, banner-grab. Слабое место: адресные структуры, синтаксис сетевого кода не на автомате.
- **Английский** — B2.
- **Инструментальные:** Python (школьный базис) — LLM-обвязка, SAST, CTF; Go (0) — eBPF user-space, DevSecOps-тулинг. Учим по необходимости, без отдельных учебных блоков.

**Блоки:**
- **A — Linux internals + Data Structures (C)** — ✅ закрыт (структура пройденного → [archive.md](archive.md)).
- **B — Сетевой стек (C)** — 🔵 активный, 7/26.
- **C — Rust (параллельный трек)** — 🔵 активный, 5/?.

### Активный блок B — Сетевой стек (7/26)

**Структура:** 26 заданий (20 атомарных + 5 интеграционных + 1 магнум опус), интеграционные на 5/10/15/20/25, опус — 26. Карта тем зафиксирована 2026-05-25 (стратегия → "### Блок B"). Базовая структура растягивается под глубину темы (скилл `plan-block`).

**Магнум опус (B-26):** network scanner — ARP discovery + ICMP ping sweep + TCP connect scan + TCP SYN scan + service detection. Отдельная репа на GitHub.

**Позиция:** 7/26 (B-1…B-7 — детали в [archive.md](archive.md), таблица заданий в [journal.md](journal.md)). Последний task — task-b07 (`epoll_echo`, 2026-06-18).

**Следующий шаг — B-8 (атомарное, Connect-with-timeout pattern):** non-blocking `connect` → epoll на `EPOLLOUT` → `getsockopt(SO_ERROR)` для проверки успеха. Классический паттерн для port scanner'а. Синтез B-6 (non-blocking connect/`EINPROGRESS`) + B-7 (epoll/`EPOLLOUT`). Фаза 2 блока. Хедеры: `<sys/socket.h>`, `<sys/epoll.h>`, `<fcntl.h>`, `<errno.h>`. Код: `networking/`.

### Параллельный блок C — Rust (5/?)

**Структура:** Ведётся линейно по главам The Book — одна глава = одно (обычно атомарное) задание `C-N`. Под структуру «4 атомарных + интеграционное + опус» не переводится: правило «каждое 5-е интеграционное» к Rust-треку не применяется (скилл `plan-block`).

**Позиция:** C-5 завершён (главы 7–8 — modules + collections, `scan_aggregator`). **Следующий шаг — C-6 (глава 9):** error handling — `Result`/`?` (закрывает вопрос 4 из [[guessing-game-notes]]). Дальше по карте: гл.10 (traits/generics/lifetimes), гл.11 (тесты), гл.12 (minigrep). Карта блока — стратегия → "### Блок C".

### Невыполненные закрепляющие задания

Пусто

### Параллельные блоки/треки доступны сейчас

- **Блок C — Rust** (см. выше). Следующий шаг — C-6 (глава 9, error handling). **По ритму 3:1 — рекомендуемый следующий шаг.**
- **Главный трек — B-8** (Connect-with-timeout: non-blocking connect + epoll `EPOLLOUT` + `getsockopt(SO_ERROR)`).
- **Мок-собес / reading-code / CTF** — управляются скиллами (по запросу пользователя или решению Claude), не счётчиками. См. таблицу триггеров в [CLAUDE.md](../../.claude/CLAUDE.md).

При запросе задания `give-task` обязан учитывать что пользователь может выбрать главный трек **или** параллельный. Если параллельные блоки доступны — спросить какой трек.
