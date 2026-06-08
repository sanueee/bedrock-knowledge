---
task: task-b-interview03-reinforce
title: sockmode_demo — тактильное закрепление матрицы blocking/non-blocking (Interview 03)
type: reinforce
status: in-progress
date: 2026-06-09
связано:
  - "[[nonblocking-poll]]"
  - "[[tcp-sockets]]"
---

## Задание

Закрепляющее после **Interview 03** (слабое место: модель blocking vs non-blocking + роль `poll`, путаница «таймаут делает сокет блокирующим», исход «hang vs мгновенная сдача» воспринимается как недетерминированный).

**Перед стартом перечитать:** [[nonblocking-poll]] — матрица 2×2 и раздел «три роли».

**Цель — не утилита, а ощутить руками** разницу трёх строк матрицы: один и тот же `recv` на **молчащем** сервере ведёт себя по-разному в зависимости от режима сокета. Замер времени делает разницу видимой.

### `networking/sockmode_demo.c`

1. Поднять «молчащий сервер»: `socket` → `bind` на `127.0.0.1:<port>` → `listen`. Клиент к нему подключится, но сервер **никогда ничего не пришлёт** и не вызовет `accept`-чтение — он просто молчит. (Достаточно `listen`; connect завершится по three-way handshake, а данных не будет.)
2. В одном процессе открыть **клиентский** сокет, подключиться (`connect`) и прогнать `recv` в **трёх конфигурациях**, замеряя elapsed по каждой (`clock_gettime(CLOCK_MONOTONIC)` до/после):

   | # | Конфигурация | Ожидаемое поведение `recv` |
   |---|---|---|
   | 1 | blocking + `SO_RCVTIMEO` = 1с (`setsockopt`) | блок ~1.0с → `-1`/`EAGAIN` |
   | 2 | non-blocking (`fcntl` `O_NONBLOCK`) | `-1`/`EAGAIN` **мгновенно** (~0 мс) |
   | 3 | blocking, без таймаута, под `alarm(2)` + обработчик `SIGALRM` | виснет ~2с, пока `SIGALRM` не прервёт → `-1`/`EINTR` |

3. По каждой конфигурации напечатать: режим, elapsed (мс), `errno` (`strerror`). Глазами увидеть: **1 ≈ 1000мс, 2 ≈ 0мс, 3 ≈ 2000мс** + разные `errno` (`EAGAIN` vs `EINTR`).

### Что задание должно закрепить (чек)

- [ ] Сокет блокирующий **по умолчанию** — в конфиге 3 мы НЕ ставим ничего, и он всё равно виснет.
- [ ] `SO_RCVTIMEO` (конфиг 1) лишь **ограничивает** блокировку, не «создаёт» её.
- [ ] `O_NONBLOCK` (конфиг 2) — единственный, кто убирает ожидание → мгновенный `EAGAIN`, **не hang**.
- [ ] `EINTR` (конфиг 3) приходит от **сигнала** (`SIGALRM`), не «сам по себе».
- [ ] elapsed-числа = тактильное доказательство «кто ждёт и сколько».

### Хедеры

`<stdio.h>`, `<stdlib.h>`, `<string.h>`, `<errno.h>`, `<unistd.h>` (`alarm`, `close`), `<signal.h>` (`SIGALRM`, `sigaction`), `<time.h>` (`clock_gettime`, `CLOCK_MONOTONIC`), `<sys/socket.h>` (`socket`/`bind`/`listen`/`connect`/`recv`/`setsockopt`, `SO_RCVTIMEO`), `<sys/time.h>` (`struct timeval`), `<netinet/in.h>` (`sockaddr_in`), `<arpa/inet.h>` (`inet_pton`/`htons`), `<fcntl.h>` (`O_NONBLOCK`).

Компиляция: `gcc -Wall -Wextra -o sockmode_demo sockmode_demo.c`

### Где сохранить
- Код: `networking/sockmode_demo.c`
- Закрыть **до B-7**, в связке с `/theory`-сессией по [[nonblocking-poll]].

---

## Моё решение

*(заполняется после выполнения)*

## Что узнал

*(заполняется после выполнения)*

## Ошибки и трудности

*(заполняется после выполнения)*

## Код

`networking/sockmode_demo.c`

## Связанные темы

[[nonblocking-poll]] [[tcp-sockets]] [[event-loop-epoll]]
