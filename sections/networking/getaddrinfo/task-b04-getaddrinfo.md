---
task: task-b04
title: getaddrinfo + DNS resolution — resolver
type: atomic
block-position: B-4
status: выполнено
date: 2026-06-05
---

## Задание

**Тип:** атомарное B-4 (тема: `getaddrinfo` + DNS resolution, IPv4/IPv6 abstraction, свой namespace ошибок).

> Перед началом перечитать конспект [[getaddrinfo]] — там уже разобран addrinfo linked list, паттерн `gai_strerror`, подводные камни. Это заготовка, на которой строим практику.

Написать утилиту `./resolve <hostname> [service]` — мини-аналог `host` / `getent hosts`. Она резолвит имя в **список адресов** через `getaddrinfo`, итерирует связный список `struct addrinfo` и печатает по строке на каждый адрес:

```
$ ./resolve example.com http
example.com -> http
  IPv4   93.184.216.34      port 80
  IPv6   2606:2800:220:1:248:1893:25c8:1946   port 80

$ ./resolve localhost
localhost -> (default)
  IPv4   127.0.0.1          port 0
  IPv6   ::1                port 0
```

`service` — необязательный второй аргумент: имя сервиса (`http`, `ssh`, `6379`) или числовой порт. Если не задан — передать `NULL` (порт будет 0, нам он не критичен — резолвим адрес).

### Что обязательно должно быть

1. **`hints` через `memset(&hints, 0, sizeof(hints))`** — занулить перед заполнением. Поля: `ai_family = AF_UNSPEC` (хотим и IPv4, и IPv6), `ai_socktype = SOCK_STREAM` (иначе один и тот же адрес придёт по разу на каждый socktype — TCP/UDP/raw — и список раздуется дубликатами).

2. **Ошибку резолва печатать через `gai_strerror(rv)`, НЕ через `strerror(errno)`.** Это ядро задания — у `getaddrinfo` свой namespace кодов (`EAI_NONAME`, `EAI_AGAIN`, ...), `errno` к нему отношения не имеет. Проверка: `rv != 0` → `fprintf(stderr, "%s\n", gai_strerror(rv))` → выход с ненулевым кодом.

3. **Итерация по `ai_next` до `NULL`.** Не «взять первый и забыть» — печатаем все адреса.

4. **IPv4/IPv6 abstraction руками.** `ai_addr` имеет тип `struct sockaddr *` (generic). Чтобы достать численный адрес и порт — надо посмотреть на `ai_family` и скастовать:
   - `AF_INET`  → `struct sockaddr_in *`,  адрес в `->sin_addr`,  порт в `->sin_port`.
   - `AF_INET6` → `struct sockaddr_in6 *`, адрес в `->sin6_addr`, порт в `->sin6_port`.
   
   Адрес в текст — `inet_ntop(family, &addr_struct, buf, sizeof(buf))` (буфер минимум `INET6_ADDRSTRLEN`). Порт — `ntohs(port)` (он в network byte order, как в B-2/B-3).

5. **`freeaddrinfo(info)`** в конце — список аллоцировала библиотека, освобождать обязан ты. Не забыть на пути ошибки тоже (если резолв успешен, а упал inet_ntop — всё равно free).

6. **Чистая компиляция с `-Wall -Wextra`.**

### Расширения, если останется время (по желанию, builder-материал)

- **`-n` флаг (AI_NUMERICHOST):** не ходить в DNS, требовать чтобы host был IP-литералом. Показать разницу: `./resolve -n example.com` → ошибка `EAI_NONAME`, `./resolve -n 127.0.0.1` → ок.
- **canonical name (AI_CANONNAME):** добавить флаг в `hints.ai_flags`, напечатать `info->ai_canonname` (приходит только в первом узле списка).

### Заголовки (функция → что подключить)

| Нужно для | `#include` |
|---|---|
| `getaddrinfo`, `freeaddrinfo`, `gai_strerror`, `struct addrinfo` | `<netdb.h>` |
| `struct sockaddr`, `struct sockaddr_in`, `struct sockaddr_in6`, `AF_INET`/`AF_INET6` | `<netinet/in.h>` |
| `inet_ntop`, `ntohs` | `<arpa/inet.h>` |
| socket-уровневые типы (подтягивается транзитивно, но указывать явно) | `<sys/socket.h>`, `<sys/types.h>` |
| `fprintf`, `printf` | `<stdio.h>` |
| `memset` | `<string.h>` |
| `exit`/коды возврата | `<stdlib.h>` |

### Где сохранить

Код: `sections/networking/getaddrinfo/resolve.c`.
Сборка: `gcc -Wall -Wextra -O2 sections/networking/getaddrinfo/resolve.c -o networking/resolve`.
Проверка: `./resolve example.com http`, `./resolve localhost`, `./resolve nonexistent.invalid` (должен дать `gai_strerror` → `Name or service not known`).

После кода — `/check` на ревью, потом `vault-write` дополнит конспект [[getaddrinfo]] боевыми деталями (особенно cast по `ai_family` — этого в заготовке нет).

---

## Моё решение

Линейный `main`: проверка `argc` (2–3, иначе usage в stderr + `EXIT_FAILURE`) → `memset(&hints)` + `AF_UNSPEC` / `SOCK_STREAM` → `getaddrinfo(host, service, &hints, &info)` → проверка `rv != 0` через `gai_strerror` → печать заголовка `host -> service` → цикл `for (p = info; p; p = p->ai_next)` → внутри ветка по `p->ai_family`: каст `ai_addr` к `sockaddr_in` / `sockaddr_in6`, `inet_ntop` (с проверкой на `NULL` + `perror`), `ntohs(port)`, печать `IPvN %-50s%u` → `freeaddrinfo(info)` → `return EXIT_SUCCESS`.

`service` если не задан — печатается как `"(default)"`, в `getaddrinfo` при этом уходит он же строкой (порт резолвится из имени или 0 — для задачи не критично, резолвим адрес).

Файл: `sections/networking/getaddrinfo/resolve.c`. Сборка `gcc -Wall -Wextra -O2`. Проверено на 4 сценариях: `localhost http` (`::1` + `127.0.0.1`), `example.com 80` (2×v4 + 2×v6, полные адреса), `nonexistent.invalid` (gai_strerror, exit 1), без аргументов (usage, exit 1).

## Ключевые функции

| Функция / тип | Зачем |
|---|---|
| `getaddrinfo(node, service, hints, res)` | Резолв «имя + сервис → список адресов». node → DNS/hosts; service → `/etc/services` или число (БЕЗ DNS). Комбинирует: каждому адресу проставляет порт. |
| `hints` (`AF_UNSPEC` + `SOCK_STREAM`) | `AF_UNSPEC` = и v4, и v6. `SOCK_STREAM` — **фильтр по протокольной колонке `/etc/services`**, чтобы один адрес не пришёл дубликатами на каждый socktype. |
| `gai_strerror(rv)` | Расшифровка кодов `getaddrinfo` — свой namespace (`EAI_NONAME`...), **не** `errno`. Поэтому `strerror(errno)` тут даст мусор. |
| каст `ai_addr` по `ai_family` | `ai_addr` = generic `struct sockaddr *`. Сначала читаешь метку `ai_family` (type tag), потом кастуешь к `sockaddr_in` / `sockaddr_in6`. Каст не двигает байты — выбирает схему чтения. |
| `inet_ntop(family, &addr, buf, len)` | Бинарный адрес → текст. `buf` ≥ `INET6_ADDRSTRLEN` (46). Возвращает `NULL` при ошибке — проверять. |
| `ntohs(port)` | Порт в структуре — network byte order. `n→h short`. |
| `freeaddrinfo(info)` | Список аллоцировала библиотека (exact-fit под каждый узел) — освобождать обязан вызывающий. |

## Грабли этой сессии

1. **`%.20s` вместо `%-46s`** — хотел выровнять колонку, но взял **precision** (точка = максимум, *обрезает*) вместо **width** (без точки = минимум, *дополняет*). Полный IPv6 рубился на 20-м символе. Правило: точка переключает width→precision; `-` = выравнивание влево. См. [[posix-naming]] (нет) — это в [[getaddrinfo]] / общий C.
2. **Copy-paste IPv6-ветки** (главный баг → **SIGSEGV**): скопировал v4-блок, не переименовал `v4`→`v6`. В v6-ветке `v4 == NULL` → разыменование NULL → segfault на первом же `localhost` (`::1`). Компилятор предупреждал: `v6 set but not used` — этот варнинг почти всегда = «скопировал и забыл переименовать». Тестировал, видимо, только IPv4 → не поймал.
3. **Рефлексия пользователя:** сама логика не была сложной. Трудность — **обилие структур/типов**: не всегда сразу вспоминается, какую из `sockaddr` / `sockaddr_in` / `sockaddr_in6` / `in_addr` / `in6_addr` / `addrinfo` использовать и какие у них поля. → завели глоссарий имён [[posix-naming]].
