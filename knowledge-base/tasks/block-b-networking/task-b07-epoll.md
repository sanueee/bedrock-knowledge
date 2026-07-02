---
тема: epoll — многоклиентский echo-сервер на event loop
блок: B — Сетевой стек
type: atomic
block-position: B-7
дата: 2026-06-09
статус: выполнено (2026-06-18)
связано:
  - "[[event-loop-epoll]]"
  - "[[nonblocking-poll]]"
  - "[[tcp-sockets]]"
код: networking/epoll_echo.c
---

# B-7 (atomic) — Multiplexing: epoll

## Задание

Написать **однопоточный многоклиентский echo-сервер** `networking/epoll_echo.c` на базе
`epoll`. Один поток обслуживает много клиентов одновременно через event loop — без
`fork`, без `pthread`, без `poll`. Это то самое «в бою», под что закладывалась тема:
non-blocking сокеты + механизм мультиплексирования работают вместе.

**Что делает программа:**
- Слушает TCP-порт (аргумент `argv[1]`, по умолчанию, скажем, 8080).
- Принимает любое число клиентов одновременно.
- Всё, что клиент прислал, шлёт ему обратно (echo).
- Корректно закрывает соединение при EOF (клиент сделал `close`) и убирает fd из epoll.
- По `SIGINT` — чистый выход (закрыть epoll fd и listen fd).

### Обязательные требования (= темы, на которых тренируемся)

1. **`epoll_create1(0)`** — создать epoll-инстанс, получить `epfd`.
2. **listen-сокет в non-blocking + регистрация в epoll на `EPOLLIN`.**
   - `socket` → `setsockopt(SO_REUSEADDR)` → `bind` → `listen` (повтор B-2).
   - Перевести в non-blocking идиомой `fcntl(F_GETFL)` → `| O_NONBLOCK` → `F_SETFL`
     (не затирать прочие флаги — см. [[nonblocking-poll]]).
   - `epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev)` где `ev.events = EPOLLIN`,
     `ev.data.fd = listen_fd`.
3. **Главный цикл — `epoll_wait`:**
   - `int n = epoll_wait(epfd, events, MAX_EVENTS, -1);` (блокируемся здесь — это ОК,
     это единственное место, где поток спит).
   - Обработать `n == -1 && errno == EINTR` (сигнал прервал) — `continue`, не падать.
   - Пройтись по `events[0..n)`.
4. **Различить два типа готового fd:**
   - `events[i].data.fd == listen_fd` → **новое соединение**: в цикле `accept` до
     `EAGAIN` (могло накопиться несколько коннектов на один `EPOLLIN`), каждый
     принятый клиент → non-blocking → `EPOLL_CTL_ADD` на `EPOLLIN`.
   - иначе → **данные от клиента**: читать `recv` до `EAGAIN`/`0`, прочитанное
     отправить обратно.
5. **Снятие клиента:** при `recv == 0` (EOF) или реальной ошибке —
   `epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL)` + `close(fd)`.
6. **Level-triggered (по умолчанию)** — НЕ ставь `EPOLLET` в этой задаче. Сначала
   почувствуй LT (событие повторяется, пока есть данные). В разборе после кода
   обсудим, что изменилось бы при ET и почему ET требует дочитывать до `EAGAIN`
   обязательно (в LT — желательно, но не смертельно).

### На что обратить внимание (узлы, ради которых задача)

- **`recv`-цикл до `EAGAIN`** — даже в LT читай в цикле, а не один раз: иначе на
  каждый байт лишний оборот event loop. Идиома цикла — в [[event-loop-epoll]].
- **`accept`-цикл до `EAGAIN`** — тот же принцип на listen-сокете.
- **`ev.data.fd`** — как ты узнаёшь, какой fd сработал. Положи туда fd при ADD,
  читай из `events[i].data.fd` после wait. (Есть union `data` — пока используем `.fd`.)
- **EOF vs ошибка vs EAGAIN** на `recv` — три ветки, как в B-5/B-6
  (`0` = клиент ушёл; `-1 && EAGAIN` = данных пока нет, выйти из read-цикла;
  `-1 && EINTR` = повтор; прочее = реальная ошибка).
- **`SIGPIPE`** — если будешь писать в закрытый клиентом сокет, прилетит `SIGPIPE`.
  Защита: `signal(SIGPIPE, SIG_IGN)` или `sigaction(SIG_IGN)` (повтор B-2), и
  обрабатывать `EPIPE` на `send`.

### Хедеры (подключить явно)

```c
#include <sys/epoll.h>     // epoll_create1, epoll_ctl, epoll_wait, struct epoll_event, EPOLLIN
#include <sys/socket.h>    // socket, bind, listen, accept, setsockopt, recv, send, SO_REUSEADDR
#include <netinet/in.h>    // struct sockaddr_in, htons, INADDR_ANY
#include <arpa/inet.h>     // inet_ntop (если печатаешь адрес клиента)
#include <fcntl.h>         // fcntl, F_GETFL, F_SETFL, O_NONBLOCK
#include <unistd.h>        // close, read/write если будешь ими
#include <signal.h>        // sigaction / signal, SIG_IGN, SIGINT, SIGPIPE
#include <errno.h>         // errno, EAGAIN, EWOULDBLOCK, EINTR, EPIPE
#include <string.h>        // memset, strerror
#include <stdio.h>         // perror, fprintf, printf
#include <stdlib.h>        // exit, atoi
```

### Проверка

1. Собрать: `gcc -Wall -Wextra -o epoll_echo epoll_echo.c`
2. Запустить сервер: `./epoll_echo 8080`
3. В **двух-трёх** разных терминалах одновременно:
   `nc localhost 8080` — печатать строки, убедиться что каждый клиент получает
   свой echo, и клиенты не блокируют друг друга.
4. Закрыть один `nc` (Ctrl+C / Ctrl+D) — сервер должен снять только его fd,
   остальные продолжают работать.
5. `Ctrl+C` по серверу — чистый выход.

### Где сохранить

Код: `networking/epoll_echo.c`. Конспект (заготовка уже есть, дополнить по итогам
через `vault-write`): [[event-loop-epoll]].

---

## Моё решение

Каркас (non-blocking listen → `epoll_create1` → ADD на `EPOLLIN` → цикл `epoll_wait`
→ accept-цикл до `EAGAIN` / recv-ветка) написал сам в первой итерации (2026-06-09).
Механические фиксы по ревью (fd-leak `close`, `EPOLL_CTL_DEL` с `NULL`) доделал сам.
Структурные узлы (EPOLLOUT-backlog и обработку `EINTR` на `recv`) и `SIGINT`-выход —
написал Claude по явной просьбе (тема новая), с пошаговым разбором логики.

## Ключевые функции

| Функция | Зачем |
|---------|-------|
| `epoll_create1(0)` | создать epoll-инстанс |
| `epoll_ctl(ADD/MOD/DEL)` | подписать/перевесить/снять fd; `DEL` принимает `NULL` |
| `epoll_wait` | единственное место сна потока; `-1/EINTR` → пере-проверить флаг останова |
| `fcntl(F_GETFL/F_SETFL, O_NONBLOCK)` | non-blocking, не затирая прочие флаги |
| `send`/`recv` non-blocking | три исхода: `>0` / `0`=EOF / `-1`+`EAGAIN`/`EINTR` |
| `sigaction` (без `SA_RESTART`) | SIGINT прерывает `epoll_wait` → чистый выход |

## Что узнал

- **Диспетчер по `events[i].events`** — событие ≠ «читать»; проверять биты `EPOLLIN/OUT/ERR/HUP`.
- **EPOLLOUT-backlog** — недописанный `send` буферизуется per-client, дослать по `EPOLLOUT`;
  busy-loop на `send` запрещён.
- **Flow-control размен** — читать новую порцию только когда прошлая отправлена (бороться с
  ростом backlog), платя узлом «drain до EAGAIN».
- **Reuse fd** — `conns[fd]` сбрасывать при `close`, иначе stale state протечёт в новый сокет.
- **`sigaction` без `SA_RESTART`** — иначе syscall перезапустится сам и флаг останова не проверится.

## Ошибки и трудности

- **По словам пользователя: логика event-loop-сервера нова целиком + большой объём нового
  синтаксиса.** Главная сложность — не отдельный вызов, а сборка модели «спим на epoll_wait,
  всё остальное не блокирует» в голове.
- FIX 4 (fd-leak) сначала закрыл только на пути `setsockopt`, забыл `bind`/`listen`.
- Самотест big-payload завис в собственном deadlock (write-then-read одним клиентом) — баг
  теста, не сервера.

## Что бы сделал иначе

При следующем заходе на event-loop-сервер — сразу закладывать per-client state-структуру и
диспетчер по битовой маске, а не «recv→send в лоб»: backpressure и снятие клиента всплывают
неизбежно. Прогонять интерактивный echo сразу с конкурентным читателем.

## Код

`networking/epoll_echo.c`

## Связанные темы

[[event-loop-epoll]] [[nonblocking-poll]] [[tcp-sockets]]
