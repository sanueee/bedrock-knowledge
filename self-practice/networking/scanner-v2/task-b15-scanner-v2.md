---
type: integration
block-position: B-15
phase: 3
title: scanner v2 — discovery диапазона + идентификация сервиса
status: in-progress
date: 2026-07-15
tags: [networking, integration, epoll, connect-scan, fingerprint, http-parsing, binary-parsing, memmem]
---

# B-15 — scanner v2 (discovery + service identification)

**Тип:** интеграционное (позиция 15/26, Фаза 3). Первое интеграционное Фазы 3.
**Синтез опоры (4 темы):**
- **B-10** (`self-practice/networking/port-scan-v1/portscan.c`) — epoll connect-scan диапазона, bounded concurrency (WINDOW слотов) → **фаза 1: discovery**.
- **B-13** (`self-practice/networking/service-fingerprint/fingerprint.c`) — библиотека `const probe`, `probes_for_port`, server/client-first, `run_probe`, memmem-матчинг → **фаза 2: identification**.
- **B-11** (`topics/networking/http.md`) — HTTP framing, граница `\r\n\r\n`, парсинг заголовков → извлечь `Server:` из HTTP-ответа.
- **B-12** (`topics/networking/dns.md`) — byte-exact парсинг с guard'ами на границы → вытащить версию из баннера ровно по `res` байтам.

**Готовит:** к магнум опусу B-26 (network scanner) — это его ядро «open ports → что за сервис». Фазы 4–5 навесят raw sockets/ARP/SYN сверху.

Перечитать перед стартом (дисклеймер: знания выветриваются):
- `topics/networking/port-scan-scaling.md` + `topics/networking/event-loop-epoll.md` — epoll-цикл дискавери.
- `topics/networking/service-fingerprint.md` — библиотека проб и `run_probe`.
- `topics/networking/http.md` — где граница заголовков и как читать одну header-строку.
- `topics/networking/dns.md` — дисциплина «трактовать ровно `n` байт, guard перед разыменованием».

Опора в репо: `port-scan-v1/portscan.c` (фаза 1 целиком) и `service-fingerprint/fingerprint.c` (`connect_timeout`, `run_probe`, `probes_for_port`, `probe` — берутся почти дословно).

---

## Задание

Цель — `self-practice/networking/scanner-v2/scanner-v2.c`. На входе host, диапазон портов, timeout. Сканирует диапазон (как B-10), для **каждого открытого** порта поднимает пробу (как B-13) и печатает сервис + извлечённую деталь (версию/Server). Closed/filtered — только считаются.

```
usage: ./scanner-v2 <host> <port-from> <port-to> <timeout-ms>

$ ./scanner-v2 scanme.nmap.org 20 90 3000
scanning scanme.nmap.org ports 20-90 ...
port 22:  open   ssh    SSH-2.0-OpenSSH_6.6.1p1
port 80:  open   http   Apache/2.4.7
port 25:  open   smtp   220 mail ESMTP Postfix
port 53:  open   unknown
71 ports scanned: 4 open, 66 closed, 1 filtered
```

### Центральная идея — две фазы, разные модели I/O

Discovery и identification — **два раздельных прохода**, потому что у них разный I/O:

- **Фаза 1 (discovery)** отвечает только на «порт открыт?» — быстро и параллельно, non-blocking + epoll, много сокетов сразу (машина B-10). Как только `SO_ERROR == 0` — порт открыт, сокет **закрывается**, номер порта кладётся в массив открытых.
- **Фаза 2 (identification)** отвечает на «что за сервис?» — по одному порту, **свежий** blocking-коннект с `SO_RCVTIMEO` и обменом payload (машина B-13, `run_probe` сам снимает `O_NONBLOCK`).

**Дизайн-развилка (обдумай, не пропусти):** можно ли переиспользовать сокет из фазы 1 вместо нового коннекта в фазе 2? Технически да, но: (1) B-10 закрывает сокет, определив открытость через `getsockopt(SO_ERROR)`, до всякого payload; (2) хранить N живых сокетов между фазами возвращает проблему лимита fd, которую bounded-concurrency в B-10 как раз решает. Рекомендация для v2 — **не переиспользовать**: фаза 1 отдаёт список номеров портов, фаза 2 коннектится заново. Это цена одного лишнего RTT на открытый порт (их мало) ради чистого разделения. Запиши этот trade-off — на нём тебя спрошу в /check.

### Хедеры

```c
#define _GNU_SOURCE            // memmem

#include <stdio.h>             // printf, fprintf, snprintf
#include <stdlib.h>            // exit, malloc, free, strtoull
#include <string.h>            // memmem, memchr, memset, strlen, strncasecmp
#include <strings.h>           // strncasecmp (если не из string.h)
#include <stdint.h>            // uint16_t
#include <errno.h>            // errno, EINTR, EINPROGRESS, ECONNREFUSED, EMFILE, ERANGE
#include <unistd.h>           // close
#include <fcntl.h>            // fcntl, O_NONBLOCK, F_GETFL, F_SETFL
#include <time.h>             // clock_gettime, CLOCK_MONOTONIC
#include <sys/time.h>         // struct timeval (SO_RCVTIMEO)
#include <sys/socket.h>       // socket, connect, send, recv, getsockopt, setsockopt
#include <sys/types.h>       // ssize_t
#include <sys/select.h>      // select, fd_set (connect-таймаут в фазе 2)
#include <sys/epoll.h>       // epoll_create1, epoll_ctl, epoll_wait (фаза 1)
#include <netdb.h>           // getaddrinfo, freeaddrinfo, gai_strerror
```

### Декомпозиция — выводишь сам (Claude помогает словами)

Готовых сигнатур здесь нет намеренно: тренируем навык «разбить задачу на функции». Разложи сам, отвечая на вопросы ниже; Claude ведёт диалогом, не выдаёт список.

- **Границы фаз.** Сколько тут раздельных работ? Что фаза 1 обязана *отдать* фазе 2 — весь результат скана или только один тип? В каком контейнере это передать и кто им владеет (кто выделяет/освобождает)?
- **Что переносится готовым из B-13.** Какие функции `fingerprint.c` берутся почти дословно — что у них уже за вход/выход, и подходят ли они фазе 2 как есть?
- **Что переносится из B-10.** Дискавери-цикл — что в нём меняется? Раньше он *печатал* «opened»; теперь его результат нужен дальше. Как это меняет то, что функция принимает и возвращает?
- **Новое (B-11/B-12).** Извлечение детали из ответа — одна функция или две (HTTP-заголовок vs текстовый баннер)? Что каждая принимает, чтобы работать по длине `res`, а не по `strlen`? Куда пишет результат — возвращает указатель или пишет в переданный буфер (и почему второе безопаснее)?
- **Точка входа.** Что делает `main` после парсинга аргументов — какой минимум вызовов, чтобы фазы состыковались?

Сформулируй свой список функций (имя + что принимает + что возвращает) — разберём каждую по очереди.

### Алгоритм (пошагово)

**Фаза 1 — discovery (перенос B-10):**
1. Парсит host + `from`/`to`/`timeout`, готовит `sockaddr_in`: `strtoull()` (валидация 1..65535), `inet_pton()`, `memset()`+`sin_family`.
2. Крутит epoll-цикл со слотами: `epoll_create1()`, non-blocking `socket()`+`connect()` (EINPROGRESS), `epoll_wait()`, `getsockopt(SO_ERROR)`.
3. При `SO_ERROR == 0` — **не печатать**, а `out_ports[k++] = port`; `close(fd)`. Дедлайн → filtered++, ECONNREFUSED → closed++.

**Фаза 2 — identification (перенос B-13), для каждого открытого порта:**
4. Заново коннектится с таймаутом: `connect_timeout()` (getaddrinfo + non-blocking connect + select).
5. Выбирает пробы по порту: `probes_for_port()`; если `n == 0` → печатает `unknown`, `close()`, next.
6. Гоняет пробу: `run_probe()` (снимает `O_NONBLOCK`, ставит `SO_RCVTIMEO`, при client-first — `send()`, потом `recv()`).
7. Матчит паттерн по содержимому ровно `res` байт: `memmem(buf, res, pattern, strlen(pattern))`.
8. Извлекает деталь: для `http` — `extract_http_server()` (граница `\r\n\r\n`, строка `Server:`); иначе — `extract_banner_version()` (до CR/LF).
9. Печатает строку `port N: open <service> <deталь>`; `close(fd)`.

**Итог:** печатает сводку `<M> ports scanned: <open> open, <closed> closed, <filtered> filtered`.

## Примеры для теста

- **Тест 1 (happy path, реальный хост)** — SSH+HTTP видны с версиями — `./scanner-v2 scanme.nmap.org 20 90 3000`
- **Тест 2 (локальные сервисы)** — подними `python3 -m http.server 8080` и проверь извлечение Server-заголовка — `./scanner-v2 127.0.0.1 8079 8081 1000`
- **Тест 3 (открыт, но не в библиотеке проб)** — порт открыт, `probes_for_port` даёт `n==0` → `unknown`, без падения — `./scanner-v2 127.0.0.1 <порт nc-listener> <...> 1000`
- **Тест 4 (edge: баннер без CRLF)** — сервис прислал строку без `\r\n` → `extract_banner_version` не должен читать за `len` (guard) — `nc -l` с ручным вводом без перевода строки
- **Тест 5 (весь диапазон закрыт)** — сводка `0 open`, без утечки fd — `./scanner-v2 127.0.0.1 1 5 500`

Чек-лист под твои слабые места ([[feedback-branch-fatigue-mechanical-slips]], [[pointer-arithmetic-ub]]):
- каждый выход из ветки фазы 2 закрывает `fd` (unknown / no-match / match — все три);
- `out_ports` не переполнить: `k < cap` guard в фазе 1;
- в `extract_*` работать по `len`, а не по `strlen(buf)` — recv не кладёт `\0`; `memchr`/`memmem` в пределах `len`, указатель `p+need` сверять с `end` **до** разыменования;
- аккумуляторы `open/closed/filtered` инициализированы нулём.

---

## Разбор /check

Много итераций. Два кластера багов: **(A) стыковка при copy-paste из v1** и **(B) byte-exact парсинг ответа** в фазе 2.

**A. Copy-paste рефакторинг из `portscan.c` (механические промахи):**
- `struct probe_port` объявлена, но **не использована**: окно активных сокетов и хранилище результатов слиты в один `long *opened_ports` — негде хранить `fd`/`deadline`. `find_free_slot` не компилировался (`slot` vs `slots`, `long*` без `.fd`). Урок: окно (`fd`+`port`+`deadline`) и результат (`opened_ports`) — **разные структуры данных**, индексируются разными счётчиками (`i` слота vs `n_opened`).
- `start_port_probe`: `close(fd)` сразу после `epoll_ctl(ADD)` + `fd`/`deadline` не сохранялись в слот.
- Внутренний `while` заполнения: `cur_port` не инкрементился, `res` не обрабатывался → зависание.
- Окно `slots[WINDOW]` объявлено **внутри** внешнего `while` → пересоздавалось каждый оборот, теряя активные сокеты. Должно жить **вне** цикла (как в v1).
- `deadline` сузил до `int` → переполнение (uptime в мс > INT_MAX). Вернул `long long`.
- **Разбор событий (`epoll_wait`) вложен во внутренний `while` заполнения** → зависание на хвосте: когда `cur_port > port_to` (порты кончились), но `active > 0`, внутренний цикл не входит, `epoll_wait` не вызывается, `active` не убывает. Разбор должен быть на уровне **внешнего** цикла (после заполнения окна) — это же и урок B-10.
- `opened_ports[c_opened]` вместо `[i]` — out-of-bounds read (`c_opened` = размер, индекс за последним валидным).
- Сигнатура `identificate_opened_ports` менялась (убрал параметр-тень `c_opened`), но правку **не довёл до вызова** в `main` — лишний аргумент. Урок: правка сигнатуры = правка всех вызовов.

**B. Извлечение детали (extract_http_server / extract_banner_version) — byte-exact:**
- `strcmp(p, " ")` на буфере от `recv` (не терминирован `'\0'`) — **out-of-bounds read (UB)**: `strcmp` бежит до `'\0'`, про `end = buf+res` не знает. Заменил на `*p == ' '` (читает ровно 1 байт по проверенному `p < end`). **Плюс** забытый `else break` → бесконечный цикл.
- `memcpy(out, buf, n)` вместо `memcpy(out, p, n)` — копировал с начала ответа, а не со значения `Server:`.
- Инверсия цикла skip-spaces: искал **первый пробел** (`if(*p==' ') break`) вместо **пропуска** ведущих (`while(*p==' ') p++`) → лишний ведущий пробел в значении.
- `memmem(buf, res, "Server: ", 7)` — паттерн 8 символов, длина 7 (рассинхрон). Согласовать через `strlen` литерала (по **литералу** `strlen` безопасен — он терминирован, в отличие от `strlen(buf)`).
- Кириллическая `т`, залетевшая в `{` при переключении раскладки → сборка упала (`'\U00000442' undeclared`). Тест шёл на **старом** бинарнике — заметно по несанированному выводу.

**Security-заметки:**
- **Недоверенные данные (untrusted data) от сервера**: `strcmp`/`strlen` по `recv`-буферу без `'\0'` = OOB-read. Работать строго по `res`: `memchr`/`memmem` с длиной, `p += k` может выскочить за `end` → сверять `p < end` **до** разыменования.
- **Санитизация не сделана (осознанно отложена)**: `%s`/`memcpy` кладут сырые байты баннера. Злонамеренный баннер с ANSI-escape (`\033[...`) → инъекция в терминал (terminal injection). Правильно — `isprint((unsigned char)c)` посимвольно при копировании в `out`, непечатаемые → `.`. TODO для доработки/B-26.

## Session-debrief / итог

**Что усвоено:**
- **Двухфазная архитектура** (модель держал верно с самого начала): discovery = non-blocking + epoll, много сокетов, отдаёт **список номеров** портов; identification = по одному порту, свежий blocking-коннект (`connect_timeout` + `run_probe`). Trade-off «не переиспользовать сокет» соблюдён.
- **Извлечение детали** — ядро B-15 (синтез B-11/B-12): развилка по `service` (`strcmp(pr.service,"http")`), HTTP → строка `Server:` (граница `\r\n`), баннер → первая строка до `\r\n`. Обе `extract_*` пишут в **переданный caller'ом буфер** (не возвращают указатель внутрь `buf` — тот повиснет после `close`/следующей пробы).
- E2E-прогон в Docker покрыл все ветки: HTTP-extract (`SimpleHTTP/0.6 Python/3.13.5`), SSH-баннер (`SSH-2.0-OpenSSH_9.6p1`), closed, filtered, `unknown` (открыт вне `probes_for_port`).

**Слабые места (подтверждены):**
- **Механические промахи при copy-paste рефакторинге** — стыковка сигнатура↔вызов, индексы `[i]` vs `[c_opened]`, уровень вложенности цикла. Не от непонимания — от переноса кода без пересборки инвариантов. См. [[feedback-branch-fatigue-mechanical-slips]].
- **Byte-exact парсинг недоверенных данных** — `strcmp`/`strlen` на recv-буфере, забытый `else break`. Горячая зона, гнать через руки, не копипастить. См. [[feedback-tedious-security-code]].

**Замечание по ходу:** задание выполнялось в разные дни — про шаг «извлечь деталь» (`extract_*`) забыл, печатал сырой ответ. Восстановил на финальной сверке с ТЗ. Причина — разрыв по времени, не невнимательность; полезно при возврате к отложенному task свериться с критериями приёмки перед «готово».

**Ключевые термины (English):** service identification, banner grabbing, byte-exact parsing, untrusted data, out-of-bounds read (OOB), terminal injection (ANSI-escape), buffer capacity vs length, dangling pointer, HTTP header framing.

**Не доделано (осознанно):** санитизация недоверенных байт (`isprint`); формат сводки отличается от ТЗ (`results:` вместо `<M> ports scanned:`) — косметика.
