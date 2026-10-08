---
type: atomic
block-position: B-6
тема: Non-blocking I/O — fcntl(O_NONBLOCK), EAGAIN/EWOULDBLOCK, EINPROGRESS, SO_RCVTIMEO
блок: B — Сетевой стек (C)
дата: 2026-06-07
статус: выполнено
код: self-practice/networking/nonblocking-io/nbconnect.c
связано:
  - "[[tcp-sockets]]"
  - "[[nonblocking-poll]]"
  - "[[event-loop-epoll]]"
  - "[[getaddrinfo]]"
---

# Задание B-6 (атомарное) — Non-blocking I/O: `nbconnect`

Фаза 2 блока B (non-blocking + connect-scan). Это фундамент под B-7 (epoll), B-8 (connect-with-timeout) и магнум опус (TCP connect scan).

**Перед стартом перечитать:**
- [[tcp-sockets]] — read timeout `SO_RCVTIMEO`, различение `EAGAIN` vs `EINTR` (B-5).
- [[event-loop-epoll]] — разделы «Почему `O_NONBLOCK` обязателен», «Readiness ≠ correctness», идиома read-loop с `EAGAIN`. Здесь уже описаны `EINPROGRESS` + `getsockopt(SO_ERROR)` концептуально — задание превращает это в код.

---

## Что написать

`self-practice/networking/nonblocking-io/nbconnect.c` — TCP-клиент, который **подключается в неблокирующем режиме** и грабит баннер, не зависая ни на одном шаге.

Это переписанный `host_probe` (B-5), но вместо блокирующего `connect` + `SO_RCVTIMEO` — **non-blocking сокет**. Цель не «лучше», а **прочувствовать механику O_NONBLOCK на connect и на recv**.

### Запуск

```
./nbconnect <host> <port> [timeout_ms]
```
- `host` / `port` — резолвить через `getaddrinfo` (мышца B-4: hints `AF_UNSPEC` + `SOCK_STREAM`, перебор `ai_next`).
- `timeout_ms` — необязательный, по умолчанию 3000.

### Поток выполнения (главный путь)

1. **Резолв.** `getaddrinfo(host, port, &hints, &res)`. Перебор адресов как в B-5: первый, на котором connect завершился успехом, выигрывает.

2. **Сокет + неблокирующий режим.** Для каждого адреса:
   ```c
   int fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
   int flags = fcntl(fd, F_GETFL, 0);          // прочитать текущие
   fcntl(fd, F_SETFL, flags | O_NONBLOCK);     // добавить O_NONBLOCK, не затерев остальные
   ```
   Важно: `F_GETFL` → `|= O_NONBLOCK` → `F_SETFL`. Не писать `F_SETFL, O_NONBLOCK` напрямую (затрёшь прочие file status flags). Это та же идиома bitwise-флагов, что в [[bitwise-flags]].

3. **Non-blocking connect.** Вызвать `connect(fd, p->ai_addr, p->ai_addrlen)`. Возможны три исхода:
   - вернул `0` — соединение завершилось мгновенно (бывает на `localhost`);
   - вернул `-1` + `errno == EINPROGRESS` — **handshake пошёл в фоне**, это норма, не ошибка;
   - вернул `-1` + другой `errno` — реальная ошибка (`close`, `continue` к следующему адресу).
   Напечатать на stderr, какой из исходов случился (видеть EINPROGRESS своими глазами — половина задания).

4. **Дождаться завершения connect через `poll` на запись.**
   ```c
   struct pollfd pfd = { .fd = fd, .events = POLLOUT };
   int n = poll(&pfd, 1, timeout_ms);
   ```
   - `n == 0` → таймаут: соединение не установилось вовремя → трактовать как «недоступно/filtered», `close`, `continue`.
   - `n == -1` && `errno == EINTR` → сигнал прервал `poll`, повторить (`do/while` или цикл с retry).
   - `n > 0` → fd «готов к записи». **Но готов ≠ подключён** (см. след. шаг).

   > `poll` здесь — **минимальный примитив ожидания готовности на одном fd**. Глубокое сравнение `select`/`poll`/`epoll`, edge- vs level-triggered — это **B-7**, туда сейчас не лезем. Тут `poll` — просто «разбуди меня, когда connect доедет или истечёт таймаут».

5. **Проверить реальный результат connect: `getsockopt(SO_ERROR)`.**
   ```c
   int so_err = 0; socklen_t len = sizeof(so_err);
   getsockopt(fd, SOL_SOCKET, SO_ERROR, &so_err, &len);
   ```
   - `so_err == 0` → подключились, этот адрес выигрывает (`break` из перебора).
   - `so_err != 0` → connect провалился (`so_err` == `ECONNREFUSED` и т.п.); напечатать через `strerror(so_err)`, `close`, `continue`.

   Это и есть «**readiness ≠ correctness**»: ядро будит на `POLLOUT` и при успехе, и при провале handshake — различить можно только через `SO_ERROR`. Не пропускать этот шаг (типичный баг — считать «poll вернул POLLOUT» за «подключился»).

6. **Неблокирующий recv баннера — увидеть `EAGAIN`.** Сразу после успешного connect попробовать прочитать **один раз**:
   ```c
   ssize_t r = recv(fd, buf, sizeof(buf) - 1, 0);
   ```
   На молчащем сервере (или до прихода баннера) вернёт `-1` + `errno == EAGAIN` (он же `EWOULDBLOCK` — синонимы, проверять оба или через `||`). Напечатать на stderr «баннера пока нет (EAGAIN)» — это **главный контраст с блокирующим recv**, который тут бы завис навсегда (в B-5 спасал только `SO_RCVTIMEO`).

7. **Дочитать баннер через `poll` на чтение.** `pfd.events = POLLIN`, тот же `timeout_ms`. По готовности — `recv` в цикле-идиоме:
   ```c
   for (;;) {
       ssize_t r = recv(fd, buf, sizeof(buf) - 1, 0);
       if (r > 0)              { buf[r] = '\0'; печатать; ... }
       else if (r == 0)        { /* сервер закрыл (EOF) */ break; }
       else if (errno == EINTR)  continue;
       else if (errno == EAGAIN) break;   // данных пока нет
       else                    { perror("recv"); break; }
   }
   ```
   Если за `timeout_ms` ничего не пришло — «баннера нет», это штатный исход (как в B-5).

8. **Уборка.** `close(fd)`, `freeaddrinfo(res)`. Помни про lifetime: после `freeaddrinfo` не трогать `p`/`res` (dangling — грабли из B-5).

---

## Что обязательно потрогать (чек закрытия темы)

- [ ] `fcntl(F_GETFL)` → `| O_NONBLOCK` → `F_SETFL` — установка флага, не затерев остальные.
- [ ] `connect` на non-blocking сокете возвращает `-1` + `EINPROGRESS`, и это **не ошибка**.
- [ ] `poll(POLLOUT)` для ожидания завершения connect с таймаутом + обработка `EINTR`.
- [ ] `getsockopt(SO_ERROR)` — отличить реально-подключился от woke-up-but-failed (readiness ≠ correctness).
- [ ] non-blocking `recv` отдаёт `-1` + `EAGAIN`/`EWOULDBLOCK` вместо блокировки.

## Стретч (если останется время) — контраст двух подходов

Добавить флаг/режим, который делает то же самое **блокирующим** сокетом + `SO_RCVTIMEO` (путь B-5), и в комментарии/выводе зафиксировать разницу:
- **non-blocking + poll** — сокет никогда не спит в syscall, ожиданием рулит `poll`/таймаут; масштабируется на тысячи fd одним потоком (дорога к epoll, B-7).
- **blocking + SO_RCVTIMEO** — проще кода, но каждый сокет «занимает» поток на время ожидания; для одного хоста ок, для скана диапазона — нет.

Это прямо отвечает на «зачем вообще O_NONBLOCK» перед тем, как в B-7/B-8 строить connect-scan.

---

## Хедеры (подключить явно)

| Функция / тип | Хедер |
|---|---|
| `fprintf`, `snprintf`, `perror` | `<stdio.h>` |
| `atoi`/`strtol`, `exit`, `EXIT_*` | `<stdlib.h>` |
| `memset`, `strerror` | `<string.h>` |
| `close` | `<unistd.h>` |
| `errno`, `EINPROGRESS`, `EAGAIN`, `EWOULDBLOCK`, `EINTR`, `ECONNREFUSED` | `<errno.h>` |
| `fcntl`, `F_GETFL`, `F_SETFL`, `O_NONBLOCK` | `<fcntl.h>` |
| `socket`, `connect`, `recv`, `getsockopt`, `setsockopt`, `SOL_SOCKET`, `SO_ERROR`, `SO_RCVTIMEO`, `socklen_t` | `<sys/socket.h>` |
| `getaddrinfo`, `freeaddrinfo`, `gai_strerror`, `struct addrinfo` | `<netdb.h>` |
| `poll`, `struct pollfd`, `POLLIN`, `POLLOUT` | `<poll.h>` |
| `struct timeval` (для `SO_RCVTIMEO` в стретче) | `<sys/time.h>` |
| `inet_ntop`, `ntohs` (печать адреса узла, опц.) | `<arpa/inet.h>` |

Компиляция: `gcc -Wall -Wextra -o nbconnect nbconnect.c`

## Тестовые цели

- `./nbconnect example.com 80` — должен подключиться (EINPROGRESS → POLLOUT → SO_ERROR=0); HTTP-сервер молчит без запроса → увидишь `EAGAIN`, потом таймаут «баннера нет».
- `./nbconnect <ssh-хост> 22` — SSH присылает баннер первым → увидишь `SSH-2.0-...`.
- `./nbconnect 127.0.0.1 1` (закрытый порт) — connect завершится с `ECONNREFUSED` через `SO_ERROR`.
- `./nbconnect 10.255.255.1 80` (недостижимый, если есть) — `poll` упрётся в таймаут (filtered).

---

## Где сохранить

- Код: `self-practice/networking/nonblocking-io/nbconnect.c`
- Vault: этот файл (`task-b06-nonblocking-io.md`) — разделы ниже заполнит `vault-write` по итогам.

## Моё решение

Переписал `host_probe` на non-blocking. Итоговая структура — **две фазы** (до неё дошёл не сразу, см. трудности):

- **Фаза 1 — найти подключённый fd.** Цикл по `ai_next`: `socket` → `fcntl(O_NONBLOCK)` → `connect`. Развилка `connect`: `res == 0` (мгновенно) → `found=1; break`; `res == -1 && EINPROGRESS` → `poll(POLLOUT)` → `getsockopt(SO_ERROR)` → `so_err==0` → `found=1; break`, иначе `close; continue`; прочий `errno` → `close; continue`. Носитель результата — флаг `found` + `fd` (аналог «`p==NULL` после цикла» из B-5).
- **Фаза 2 — прочитать баннер.** Только если `found`: `poll(POLLIN)` (ожидание с таймаутом) → `recv`-цикл (вычерпать поток до `EAGAIN`/`0`) → `close(fd)`.

Демо-recv «увидеть EAGAIN» из задания в финале **убрал** — он дрался с боевым recv-циклом за роль читателя (приводил к use-after-close). Цикл сам ловит `EAGAIN`, отдельное наблюдение не нужно.

Проверено вживую: `127.0.0.1:1` → `Connection refused` (через `SO_ERROR`); `example.com:80` → подключился, молчит, чистый выход без баннера; `github.com:22` → `SSH-2.0-...` прочитан.

## Ключевые функции

| Функция | Зачем |
|---------|-------|
| `fcntl(F_GETFL)` → `\| O_NONBLOCK` → `F_SETFL` | перевести сокет в non-blocking, не затерев прочие file status flags |
| `connect` (non-blocking) | стартует handshake и сразу возвращает `-1`/`EINPROGRESS` (fire-and-forget) |
| `poll(POLLOUT, timeout)` | дождаться **завершения** connect (или таймаута) |
| `getsockopt(SOL_SOCKET, SO_ERROR)` | узнать **результат** async connect (readiness ≠ correctness); read-and-clear |
| `poll(POLLIN, timeout)` | дождаться **прихода** данных (на non-blocking сокете `recv` сам не ждёт) |
| `recv` в цикле | вычерпать поток до `EAGAIN` (нет данных сейчас) / `0` (EOF) |

## Что узнал

- **`EINPROGRESS` ≠ `EINTR`.** `EINTR` = «вызов прервали до работы, повтори». `EINPROGRESS` = «работа уже идёт в фоне, НЕ повторяй, иди жди». Зациклить `connect` на `EINPROGRESS` — получить `EALREADY` на втором вызове.
- **Два канала ошибок connect.** Синхронный `errno` (вызов вернул `-1` прямо сейчас) vs асинхронный `SO_ERROR` (handshake провалился в фоне, когда ни один syscall не был активен — ошибке некуда деться, кроме как на сокет). `SO_ERROR` читается **один раз** и обнуляется.
- **На non-blocking сокете ждать негде, кроме `poll`.** `recv`/`connect` сами не блокируют. Без `poll` recv-цикл сдаётся мгновенно по `EAGAIN` — баннер теряется (это **не** зависание; зависание — свойство блокирующего режима). Гарантию «не висеть вечно» даёт **таймаут, переданный в `poll`**, а не `poll` сам по себе.
- **Структура важнее синтаксиса.** Запутанность кода была не в отдельных строках, а в том, что один цикл делал две работы. Разделение на фазы убрало асимметрию веток.

## Ошибки и трудности

- **Главная (субъективно, ответ на вопрос vault-write):** blocking vs non-blocking сокеты и роль `poll` всё ещё **в тумане** — это уже 2-3-е задание с темой (B-2 partial read, B-5 SO_RCVTIMEO, теперь B-6), но автоматизма нет. Требуется отдельная `/theory`-сессия + ещё практика. См. [[nonblocking-poll]] (написан прицельно под этот пробел).
- Применил retry-идиому B-5 (`do/while errno==EINTR`) к `EINPROGRESS` → `connect` зациклился. Концептуально не различал «прервано» и «в процессе».
- Не сразу увидел, что цикл делает две работы → ветки `break`/`close;continue`/проваливание конфликтовали; путь `res==0` сначала не ставил `found`; демо-recv закрывал fd и ронял фазу 2 в use-after-close.
- Повторы из ревью (низкий приоритет, не блокеры): `fcntl(F_GETFL)` без проверки возврата; `r == - 1` спейсинг; диагностика в stdout без `\n`.

## Что бы сделал иначе

- Сразу проектировать «найти соединение / использовать соединение» как две фазы — паттерн `_anetTcpGenericConnect` из B-5 уже это показывал.
- Не тащить «демо-recv» в финал — наблюдение `EAGAIN` сделать одноразовым логом, не путём управления.

## Код

`self-practice/networking/nonblocking-io/nbconnect.c`

## Связанные темы

[[nonblocking-poll]] [[tcp-sockets]] [[event-loop-epoll]] [[getaddrinfo]] [[bitwise-flags]]
