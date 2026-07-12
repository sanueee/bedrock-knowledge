---
type: atomic
block-position: B-13
phase: 3
title: Service fingerprinting — библиотека проб (SSH/HTTP/SMTP/FTP)
status: in-progress
date: 2026-07-12
tags: [networking, service-detection, banner-grab, recv, send]
---

# B-13 — Service fingerprinting

**Тип:** атомарное (позиция 13/26, Фаза 3).
**Синтез опоры:** connect-с-таймаутом (B-8), `send`/`recv` (B-2), парсинг ответа сервера (B-11/B-12).
**Готовит:** пробы для интеграционного B-15 (scanner v2 — identification сервиса на каждом открытом порту).

Перечитать перед стартом: `topics/networking/connect-timeout-scan.md`, `topics/networking/http.md`, `topics/networking/tcp-sockets.md`.
Опора в репо: `sections/networking/host-probe/host_probe.c` (banner-grab одного порта, SO_RCVTIMEO), `sections/networking/connect-timeout/connect_timeout.c`.

---

## Задание

Написать `sections/networking/service-fingerprint/fingerprint.c` — утилиту, которая по `<host>` и списку портов
для каждого порта: подключается, применяет **пробу, соответствующую этому порту**,
и по ответу классифицирует сервис (`ssh` / `http` / `smtp` / `ftp` / `unknown`).

```
usage: ./fingerprint <host> <port> [port ...]

$ ./fingerprint scanme.nmap.org 22 80 25
22/tcp   ssh      SSH-2.0-OpenSSH_6.6.1p1 Ubuntu-2ubuntu2.13
80/tcp   http     Apache/2.4.7 (Ubuntu)
25/tcp   closed
```

### Ключевая идея — два класса сервисов

Разница, вокруг которой строится вся утилита:

- **Server-speaks-first** (SSH, SMTP, FTP): после `connect()` сервер сам присылает
  greeting/banner. Ничего слать не надо — сразу `recv()`.
- **Client-speaks-first** (HTTP): сервер молчит, пока клиент не пошлёт запрос.
  Сначала `send()` HTTP-пробу, потом `recv()` ответ.

Значит у пробы есть флаг «слать ли что-то перед чтением» + сама строка-проба (для HTTP)
+ паттерн, по которому опознаём сервис в ответе.

### Таблица «порт → проба → паттерн»

| Порт(ы)      | Класс              | Что слать                          | Паттерн в ответе | Сервис |
|--------------|--------------------|------------------------------------|------------------|--------|
| 22           | server-first       | ничего                             | `SSH-`           | ssh    |
| 80/8080/8443 | client-first       | `HEAD / HTTP/1.0\r\n\r\n`          | `HTTP/`          | http   |
| 25/587       | server-first       | ничего                             | `220 ` + `SMTP`/`ESMTP` | smtp |
| 21           | server-first       | ничего                             | `220 `           | ftp    |

(Матчинг: сначала — по паттерну в теле ответа, не по номеру порта. Порт лишь выбирает,
*какую пробу* послать. Сервис на нестандартном порту — норма; поэтому решение принимает
**ответ**, а не порт.)

### Что делает программа (пошагово, с вызовами)

1. Разбирает argv: host + список портов (`atoi()`/`strtol()` по каждому).
2. Для каждого порта резолвит и подключается с таймаутом:
   `getaddrinfo()` → `socket()` → неблокирующий `connect()` + `select()`/`poll()` по таймауту
   (взять из `connect_timeout.c`). Не подключился → печать `порт closed`, следующий порт.
3. Ставит таймаут на чтение: `setsockopt(SO_RCVTIMEO)` (как в `host_probe.c`).
4. Выбирает пробу по порту (таблица выше). Если проба client-first — шлёт строку:
   `send()` в цикле до полной отправки.
5. Читает ответ: `recv()` (один-два вызова, баннер короткий; таймаут ловит «сервер молчит»).
   **Трактовать ровно `n` байт** — `recv` не кладёт `\0`.
6. Классифицирует: ищет паттерн в прочитанных `n` байтах — `memmem()` (границы!),
   не `strstr()` по неполной строке.
7. Печатает `порт/tcp  сервис  <вырезанный баннер/version>`. Баннер обрезать до первой
   `\r`/`\n` и печатать только `%.*s` по длине (не `%s`).
8. `close(fd)`, следующий порт.

### Хедеры

```c
#include <stdio.h>       // printf, fprintf, snprintf
#include <stdlib.h>      // exit, strtol
#include <string.h>      // memmem, memchr, memset, strlen
#include <stdint.h>      // uint16_t
#include <errno.h>       // errno, EINTR, EAGAIN, EWOULDBLOCK, EINPROGRESS
#include <unistd.h>      // close
#include <fcntl.h>       // fcntl, O_NONBLOCK
#include <sys/time.h>    // struct timeval
#include <sys/socket.h>  // socket, connect, send, recv, setsockopt, SO_RCVTIMEO
#include <sys/types.h>   // ssize_t
#include <sys/select.h>  // select, fd_set  (если connect-таймаут через select)
#include <netdb.h>       // getaddrinfo, freeaddrinfo, gai_strerror
```

> `memmem` — GNU-расширение: нужен `#define _GNU_SOURCE` **первой строкой** файла
> (до любого `#include`), иначе implicit declaration.

### Требования

- Компилировать с `-Wall -Wextra`, чисто.
- Классификация — по **содержимому ответа**, порт только выбирает пробу.
- Ни одного `strstr`/`%s` по буферу из `recv` без учёта длины `n` (это горячая зона —
  недоверенные данные из сети, `\0` не гарантирован; см. уроки B-11/B-12).
- Проверить end-to-end: `scanme.nmap.org 22 80` (реальные ssh+http), закрытый порт,
  HTTP на своём `python3 -m http.server`, `unknown` (порт открыт, но паттерн не совпал).

### Дизайн-подсказка (структура пробы)

Минимальный каркас — тела пишешь сам. Не копируй готовое, собери по частям:

```c
struct probe {
    int          send_first;   // 1 — client-first (слать payload), 0 — server-first
    const char  *payload;      // что слать (NULL для server-first)
    const char  *pattern;      // подстрока-маркер в ответе
    const char  *service;      // имя сервиса для вывода
};

// сопоставление порта → набор проб для него
struct probe *probes_for_port(uint16_t port, size_t *n);

// connect с таймаутом, вернуть fd или -1  (из connect_timeout.c)
int connect_timeout(const char *host, uint16_t port, int timeout_ms);

// применить пробу к готовому fd: слать payload (если надо), прочитать ответ в buf
ssize_t run_probe(int fd, const struct probe *pr, char *buf, size_t buflen);
```

---

## Итог

`sections/networking/service-fingerprint/fingerprint.c` — готов, `-Wall -Wextra` чисто, оттестирован end-to-end:
SSH 22 (server-first → `SSH-2.0-OpenSSH_6.6.1p1`), HTTP 80/8080 (client-first → `Apache/2.4.7`),
closed → `connect_timeout error`, unknown-порт → `unknown`. Обе ветки `send_first`, все 4 класса.

**Архитектура:** статическая таблица `const probe` (send_first/payload/pattern/service) →
`probes_for_port(port)` выбирает пробу по порту → `connect_timeout` (неблокирующий connect +
select по таймауту + `getsockopt(SO_ERROR)`) отдаёт живой fd → `run_probe` (снять `O_NONBLOCK`,
`SO_RCVTIMEO`, при client-first `send` payload, `recv` баннер) → `memmem` матчит паттерн в `main`.
Классификация — по **содержимому** ответа, порт лишь выбирает пробу.

## Security-заметки

- **`%s`/`strstr` по буферу из `recv` — мина.** `recv` не кладёт `\0`; печать `%s` по полному
  буферу → OOB-read. Решение: `memmem(buf, res, ...)` по длине + печать `%.*s` по `res`.
- **`strlen(NULL)` на server-first пробах → SIGSEGV.** SSH/SMTP/FTP имеют `payload == NULL`;
  `strlen(pr->payload)` вне `if (send_first)` разыменовывает NULL. Урок: NULL-guard там, где
  поле опционально по дизайну (тут — `payload` осмысленно NULL для server-first).
- **Молчаливое усечение порта.** `uint16_t port = strtol(...)` усекает по модулю `2^16`
  до валидации → проверять диапазон на `long` **до** любого сужения (`chtol` возвращает `long`).
- **fd leak на каждой ветке выхода.** `continue` мимо `close(fd)` (ветка `n==0` в connect,
  `n==0 unknown` в main) → утечка дескрипторов в цикле по портам. Правило: из тела выхожу
  либо `return fd`/успех, либо `close(fd); continue` — третьего не дано.

## Разбор /check (6 итераций)

| Итерация | Найдено | Класс |
|----------|---------|-------|
| 1 | `run_probe` пуст (UB-возврат); усечение порта + мёртвая валидация `port>65535`; `%s` по recv; fd leak при `n==0` | crit |
| 2 | правка усечения ухудшила (`uint16_t value = strtol`) — усечение ещё раньше; fd leak не закрыт | рецидив |
| 3 | оба закрыты (chtol→`long`, диапазон на long; `close(fd)` в unknown) | fixed |
| 4 | `send`-цикл: `sent` не инициализирован, `sent += sent` (удвоение вместо аккумуляции), condition сравнивает возврат одного send с полной длиной | crit |
| 5 | правка перенесла баг: `total` не инициализирован (тот же класс на другой переменной) | рецидив |
| 6 (тест) | `strlen(NULL)` → SIGSEGV на SSH; `timeout_ms=3` (мс, флейк); warnings `%u`/long, sign-compare | crit (нашёл тест) |

**Ключевой урок процесса:** механические промахи (неинициализированные аккумуляторы, NULL-guard,
забытый `close`) — от усталости на объёме ветвей/error-handling, не от непонимания. Concept-стыки
(неблокирующий→блокирующий сокет для `SO_RCVTIMEO`, writable≠connected) усвоены на `/theory` до кода
и в коде не ломались. Баг `strlen(NULL)` **не** ловится компилятором и `/check` по чтению —
его выявил только прогон server-first пробы. См. [[feedback-tedious-security-code]].

## Ключевые термины (English)

- **service fingerprinting** — идентификация сервиса по ответу на пробу, не по номеру порта.
- **banner grabbing** — чтение приветственного баннера сервера (server-first протоколы).
- **server-first / client-first** — кто шлёт первым после `connect`: сервер (SSH/SMTP/FTP) или клиент (HTTP).
- **probe** — проба: payload + ожидаемый паттерн для конкретного сервиса.
- **`SO_RCVTIMEO`** — сокетная опция таймаута на блокирующий `recv`.
- **partial send** — `send` может отправить меньше запрошенного → цикл-аккумулятор.
