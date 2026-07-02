---
task: task-b-interview03-reinforce
title: sockmode_demo — тактильное закрепление матрицы blocking/non-blocking (Interview 03)
type: reinforce
status: выполнено
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

Один процесс, один файл. В начале `main`: молчащий сервер (`socket`→`bind` на `127.0.0.1`→`listen(8)`, без `accept`) + однократная установка handler `SIGALRM` через `sigaction` (`sa_flags = 0`, без `SA_RESTART`). Дальше три блока подряд, каждый — свежий клиентский сокет → настройка режима → `connect` к своему серверу → `recv` в обёртке `clock_gettime(CLOCK_MONOTONIC)` → печать `режим / elapsed_ms / strerror(errno)`.

Elapsed считается в мс из обоих полей `timespec`: `(end.tv_sec-start.tv_sec)*1000 + (end.tv_nsec-start.tv_nsec)/1000000`.

**Прогон (port 54322) — три строки материализуют матрицу:**

```
config 1  blocking + SO_RCVTIMEO=1с  → 1001 мс, EAGAIN  (спал до таймаута)
config 2  non-blocking (O_NONBLOCK)  →    0 мс, EAGAIN  (не спал вообще)
config 3  blocking + alarm(2)        → 2005 мс, EINTR   (спал до сигнала)
```

## Ключевые функции

| Функция | Зачем |
|---------|-------|
| `setsockopt(SO_RCVTIMEO)` | ограничить сон blocking-`recv` (конфиг 1) — НЕ меняет режим |
| `fcntl(F_GETFL/F_SETFL, O_NONBLOCK)` | перевести сокет в non-blocking (конфиг 2) |
| `sigaction(SIGALRM)` + `alarm(2)` | прислать сигнал через 2с, прервать blocking-сон → `EINTR` (конфиг 3) |
| `clock_gettime(CLOCK_MONOTONIC)` | замер elapsed — единственный различитель режима |
| `recv` | объект эксперимента: один и тот же вызов, три исхода по режиму |

## Что узнал

- **Различитель режима — elapsed, а не `errno`.** Конфиги 1 и 2 дают **один и тот же** `EAGAIN`, но 1001 мс против 0 мс. По коду ошибки blocking+timeout и non-blocking **неотличимы**; различает их только то, спал поток или нет. Это и был узел, на котором спотыкался на Interview 03.
- `SO_RCVTIMEO` **не меняет режим** — сокет остаётся blocking, таймаут лишь обрезает сон сверху. Сон был (1001мс) → значит blocking.
- `O_NONBLOCK` — единственный, кто убирает ожидание → `EAGAIN` мгновенно (0мс), **не hang**.
- `EINTR` приходит от **сигнала** (`SIGALRM` через `alarm`), не «сам по себе» и не «от сисколла». Без `alarm` blocking-`recv` на молчащем сервере висел бы **вечно**.
- `SA_RESTART` намеренно **выключен** (`sa_flags = 0`): иначе ядро перезапустило бы прерванный `recv`, и `EINTR` никогда бы не увидел.
- `connect` без `accept` на сервере всё равно завершается — ядро само достраивает handshake и кладёт соединение в accept-очередь.

## Ошибки и трудности

Главная трудность (со слов): **путаница blocking vs non-blocking** — она же причина Interview 03. По ходу всплыла в живом виде: в `/theory` ошибочно решил, что конфиг 1 «неблокирующий, раз вернул `EAGAIN`» (на деле `EAGAIN` — не маркер режима). Закрыто через замер времени.

Баги по итерациям `/check`:
1. `time_end.tv_sec - time_end.tv_sec` (×3) — вычитание конца из самого себя, elapsed всегда 0. Сердце задания было мертво.
2. `recv(blocking_with_timeout, ...)` в блоке non-blocking — копипаста на уже закрытом fd → `EBADF`.
3. Конфиг 3: сокет `blocking` не подключён (`connect` забыт) → `recv` дал бы мгновенный `ENOTCONN`, а не 2с hang.
4. **`EINPROGRESS` принят за ошибку** (подтверждено прогоном): non-blocking `connect` на loopback вернул `-1`/`EINPROGRESS`, код сделал `perror`+`close` → `recv` на закрытом fd → `EBADF` вместо `EAGAIN`. Чинится guard'ом `if (errno != EINPROGRESS)`. Ту же логику уже писал правильно в B-6 (`nbconnect.c`), но сюда не перенёс.

## Что бы сделал иначе

- Сохранять возврат `recv` в `ssize_t r` — `errno` достоверен только при `r == -1` (здесь всегда `-1`, но привычка правильная).
- Перенести обработку `EINPROGRESS` из B-6 сразу, а не ловить через прогон.

## Где модель закрепится окончательно

Со слов: написать **многоклиентский сервер**, который держит много соединений и отвечает незамедлительно. Это ровно **B-7 (epoll / event loop)** — там non-blocking + `poll`/`epoll` работают «в бою», а не на синтетическом молчащем сокете.

## Код

`networking/sockmode_demo.c`

## Связанные темы

[[nonblocking-poll]] [[tcp-sockets]] [[event-loop-epoll]]
