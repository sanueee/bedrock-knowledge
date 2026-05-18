---
тема: TCP-сокеты в Linux / TCP sockets
блок: B — Сетевой стек
дата: 2026-05-16
связано:
  - "[[async-signal-safe]]"
  - "[[fd-kernel-model]]"
  - "[[pipes-linux]]"
  - "[[proc-net]]"
  - "[[signals-linux]]"
  - "[[syscalls-linux]]"
---

# TCP-сокеты в Linux / TCP sockets

## Что это

Socket — endpoint двунаправленного байтового канала. Для TCP — надёжный, упорядоченный, ориентированный на соединение поток байтов (`SOCK_STREAM`). Сокет в Linux это **file descriptor** — те же `read/write/close`, плюс специфичный socket API (`bind/listen/accept/connect/send/recv`).

## Ключевые термины (English)

- **socket** — endpoint, file descriptor для сетевого I/O
- **bind** — привязать сокет к локальному адресу/порту
- **listen** — перевести сокет в passive mode для приёма соединений
- **accept** — принять входящее соединение, получить новый fd
- **connect** — активно подключиться к удалённому endpoint'у
- **listening socket** vs **accepted socket** — первый порождает второй, они разные fd
- **stream socket** vs **datagram socket** — TCP vs UDP
- **passive socket** — сокет после `listen()`, не передаёт данные сам
- **peer** — другая сторона соединения
- **partial read** / **partial write** — `recv`/`send` могут вернуть меньше запрошенного, это нормально для stream
- **EOF on stream** — `recv == 0`, peer закрыл соединение (orderly shutdown, FIN получен)
- **orderly shutdown** — корректное закрытие через FIN
- **RST** — грубое закрытие, флаг в TCP-заголовке
- **half-close** — одна сторона закрыла свой write-end, другая может продолжать читать
- **TIME_WAIT** — состояние TCP после закрытия, ~2*MSL (Linux ~60s), удерживает порт
- **MSL** — Maximum Segment Lifetime
- **network byte order** — big-endian, формат чисел в сетевых протоколах
- **host byte order** — порядок байтов CPU (x86/ARM — little-endian)
- **value-result parameter** — параметр которого ядро и читает, и перезаписывает (`socklen_t *`)
- **address family** — `AF_INET` (IPv4), `AF_INET6`, `AF_UNIX`, `AF_PACKET`
- **broken pipe** — попытка записи в соединение которое peer закрыл

## Жизненный цикл — сервер vs клиент

```
SERVER:                            CLIENT:
  socket()      ─► listen_fd          socket()    ─► sock
  setsockopt(SO_REUSEADDR)
  bind()
  listen()         ┌── passive
loop:              │                  connect()  ──► TCP handshake
  accept()  ──►  client_fd ◄────────┐
    loop:                           │
      recv() ◄──────────────────────┼── send()
      send() ──────────────────────►┘  recv()
    close(client_fd)                   close(sock)
  close(listen_fd)
```

`listen_fd` и `accepted_fd` — **разные сокеты**. Listening — только производит соединения, accepted — несёт данные. Закрытие accepted не закрывает listening.

## socket(domain, type, protocol)

- **domain**: семейство адресов. `AF_INET` (IPv4), `AF_INET6` (IPv6), `AF_UNIX` (IPC через файлы), `AF_PACKET` (raw Ethernet).
- **type**: модель доставки.
  - `SOCK_STREAM` — надёжный байтовый поток (с `AF_INET` ⇒ TCP).
  - `SOCK_DGRAM` — ненадёжные дейтаграммы с границами сообщений (⇒ UDP).
  - `SOCK_RAW` — ядро не оборачивает payload, сам пишешь заголовки.
- **protocol**: `0` = дефолт для `(domain, type)`. Для `(AF_INET, SOCK_STREAM)` — `IPPROTO_TCP`.

С `SOCK_DGRAM` функции `listen`/`accept` не работают — UDP без соединения. Используется `recvfrom`/`sendto`.

## sockaddr_in — заполнение

```c
struct sockaddr_in addr;
memset(&addr, 0, sizeof addr);   // обязательно — есть padding sin_zero[8]
addr.sin_family = AF_INET;        // должен совпасть с AF_INET из socket()
addr.sin_port = htons(port);      // 16 бит, big-endian
addr.sin_addr.s_addr = htonl(INADDR_ANY);  // 32 бита, big-endian
```

`bind(fd, (struct sockaddr *)&addr, sizeof addr)` — каст обязателен, sock-функции принимают `struct sockaddr *` как полиморфный тип (исторически до `void *`).

## Network byte order

CPU x86/ARM хранит числа в **little-endian** (младший байт по младшему адресу). TCP/IP-заголовки используют **big-endian** (network byte order).

- `htons(x)` — host to network, **s**hort (16 бит). Для портов.
- `htonl(x)` — host to network, **l**ong (32 бита, исторически). Для IPv4 адресов.
- `ntohs`, `ntohl` — обратные операции.

`INADDR_ANY == 0`, `htonl(0) == 0` — формально вызов лишний, но пишем всегда:
1. Привычка — если завтра вместо `INADDR_ANY` поставишь `inet_addr("192.168.1.1")`, без `htonl` уедешь не туда.
2. Стандарт может поменять константу — твой код переживёт.

Это зеркально к `/proc/net/tcp` (task-07), где наоборот разбирали big-endian hex в host order.

## SO_REUSEADDR и TIME_WAIT

После закрытия TCP-соединения **активный закрывающий** (отправивший FIN первым) переходит в `TIME_WAIT` на `~2 * MSL` (Linux ~60s).

**Зачем TIME_WAIT нужен:**
- Запоздавшие пакеты старого соединения не должны попасть в новое с теми же endpoint'ами.
- Время на переотправку финального ACK если он потеряется.

**Эффект для сервера:** `bind` на тот же порт в течение этого окна → `EADDRINUSE`. После `Ctrl-C` сервер не запустится повторно ~минуту.

```c
int val = 1;
setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof val);
```

— говорит ядру "разреши `bind` даже при существующем TIME_WAIT". Безопасно для серверов.

**Не путать с `SO_REUSEPORT`** — другая опция, позволяет нескольким процессам биндиться на один порт (load balancing).

## setsockopt — структура

`setsockopt(fd, level, optname, optval_ptr, optlen)`:
- **level**: на каком слое опция — `SOL_SOCKET` (общие), `IPPROTO_TCP` (`TCP_NODELAY`), `IPPROTO_IP`.
- **optname**: что именно — `SO_REUSEADDR`, `SO_KEEPALIVE`, `SO_RCVBUF`.
- **optval**: указатель на значение (для bool — `int`, не `bool`).
- **optlen**: `sizeof` того что лежит по `optval`.

## accept и value-result параметр

```c
struct sockaddr_in client_addr;
socklen_t len = sizeof client_addr;   // обязательно ПЕРЕД каждым accept
int cfd = accept(server, (struct sockaddr *)&client_addr, &len);
```

`socklen_t *` — **value-result parameter**:
- На вход: размер буфера который ты предоставляешь.
- На выход: ядро перезаписывает значение **реальным** размером записанного адреса.

Если объявить один раз снаружи цикла и не сбрасывать:
- С `sockaddr_in` всё стабильно (всегда 16 байт) — не заметишь.
- С `sockaddr_storage` (универсальный буфер v4/v6): после v4-клиента `len` стал 16, следующий v6-клиент (28 байт) **усечётся** — `inet_ntop` покажет мусор.

Привычка: инициализировать **перед каждым** `accept`/`recvfrom`/`getpeername`.

## recv — три исхода

```c
ssize_t n = recv(fd, buf, sizeof buf, 0);
if (n == 0)        // EOF: peer закрыл write-end (получили FIN)
if (n == -1)       // ошибка, смотри errno
if (n > 0)         // прочитали n байт
```

**`n == 0` ≠ "ничего не пришло".** `recv` блокируется пока данных нет; возвращает 0 **только** когда peer сделал orderly shutdown (`close`/`shutdown(SHUT_WR)`). Это **EOF на TCP-stream**. Реакция: закрыть `fd`, идти за следующим клиентом.

## Partial read — критично

`recv(fd, buf, N, 0)` может вернуть **любое** число от 1 до N. Не "сколько прислали", а "сколько ядро готово сейчас отдать". Это **stream**, не сообщения.

Сценарии:
- Клиент послал 50 байт, буфер 4096 → `recv` вернёт 50.
- Клиент послал 10000 байт за один `send`, буфер 4096 → `recv` вернёт ≤ 4096; остальные 5904 **остаются в kernel-буфере**, заберёшь следующим `recv`.
- Клиент послал 100 байт **двумя** `send` по 50 → `recv` может вернуть 100 одним вызовом (TCP объединил), или 50, или даже 30 + 70. Гарантии порядка байт — есть, гарантии границ сообщений — **нет**.

Следствие: **один `send` ≠ один `recv`**. Если протокол требует "сообщение длиной N" — считать байты в цикле самому, либо использовать длину-префикс / разделитель / фиксированную длину.

`send` симметрично: может записать меньше чем просили (для маленьких payload'ов почти всегда пишет всё, но `write_all` цикл — правильная привычка).

`recv` не добавляет `\0` к данным — это байты, не строка. Хочешь работать как со строкой — терминируй сам после.

## EINTR — прерывание сигналом

Медленные syscall'ы (`accept`, `recv`, `send`, `read`, `write`, `connect`, `wait`...) могут вернуть `-1` с `errno == EINTR` если во время блокировки пришёл сигнал и handler был запущен. Это **не ошибка** в обычном смысле — syscall прервался ДО того как успел что-либо сделать, данные не потеряны.

**Реакция: `continue` в обоих случаях** (accept и recv).

- На `accept`: `continue` → следующая итерация → `while(!stop)` проверит флаг → выйдем если был SIGINT.
- На `recv`: `continue` → повтор recv для того же соединения.

`SA_RESTART` в `sa_flags` отключает EINTR — ядро автоматически перезапустит syscall. **Не использовать для shutdown-сигналов** (SIGINT/SIGTERM): без EINTR `accept` никогда не вернётся, `while(!stop)` никогда не проверится, сервер не реагирует на Ctrl-C.

## EINTR на разных слоях — accept vs recv

- **`accept` ошибка не-EINTR** → `continue`. Listening сокет жив, следующий клиент может прийти. Сломалось текущее `accept`, не сервер.
- **`recv` ошибка не-EINTR** → `break` из внутреннего цикла, `close(client_fd)`. Сломалось **именно это соединение** (`ECONNRESET`, `ETIMEDOUT`, `EBADF`); никакой `continue` его не починит, данные потеряны.

Это разделение листового и клиентского сокетов: lifecycle разный.

## EPIPE и ECONNRESET — штатная ситуация

- **`EPIPE`** — peer закрыл соединение нормально (FIN), а ты пытаешься писать.
- **`ECONNRESET`** — peer оборвал грубо (RST). Типично: `kill -9` клиента, rebooт, network drop.

Это **нормальные** события для сервера, клиенты приходят и уходят неаккуратно. **`perror` не нужен** — забьёт лог. Просто `break` из внутреннего цикла, `close(client_fd)`, идём за следующим клиентом.

Контраст: `EBADF`/`ENOMEM` в `send`/`recv` — это **баг в твоём коде** или системная проблема, `perror` уместен.

## SIGPIPE и три способа защиты

**Механика смерти:**
1. Клиент `connect` → данные → `close()` или `kill -9`.
2. Сервер должен увидеть `recv == 0` или `recv == -1`.
3. Если сервер игнорирует и продолжает `send` — первый `send` уйдёт, peer ответит RST.
4. **Следующий** `send` → ядро посылает **тебе** (отправителю) `SIGPIPE`.
5. Дефолтная реакция `SIGPIPE` — `terminate`. Сервер умирает молча.

Ключ: `SIGPIPE` приходит **отправителю** ("ты пишешь в никуда"), а не peer'у.

**Три способа защиты:**

1. **`MSG_NOSIGNAL` флаг в каждом `send`** — локально. Плюс: явно, не глобальное состояние. Минус: Linux-only (на macOS/BSD — `SO_NOSIGPIPE` через `setsockopt`); легко забыть в одном `send` из десяти.

2. **`signal(SIGPIPE, SIG_IGN)` глобально** — старая POSIX-функция. Простое, портабельное.

3. **`sigaction(SIGPIPE, &sa_ign, NULL)` с `sa_handler = SIG_IGN`** — современно, безопасно (без System V автосбросов). Идиоматично рядом с `SIGINT`-обработкой.

Любой способ превращает `SIGPIPE` в `send == -1` с `errno = EPIPE`. Дальше обычная обработка.

**Антипаттерн:** ставить `handler` (свою функцию) на `SIGPIPE`. Работает, но: лишняя печать, дополнительные `EINTR` в других syscall'ах, нет смысла. Цель — **подавить** сигнал, а не ловить.

## Пример — minimal server skeleton (из task-10)

```c
int server = socket(AF_INET, SOCK_STREAM, 0);
int val = 1;
setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &val, sizeof val);

struct sockaddr_in addr = {0};
addr.sin_family = AF_INET;
addr.sin_port = htons(port);
addr.sin_addr.s_addr = htonl(INADDR_ANY);
bind(server, (struct sockaddr *)&addr, sizeof addr);

struct sigaction sa_pipe = { .sa_handler = SIG_IGN };
sigaction(SIGPIPE, &sa_pipe, NULL);

listen(server, 8);
while (!stop) {
    struct sockaddr_in caddr;
    socklen_t clen = sizeof caddr;
    int cfd = accept(server, (struct sockaddr *)&caddr, &clen);
    /* ...inner recv/send loop... */
    close(cfd);
}
close(server);
```

(Полный код — `networking/echo_server.c`.)

## Подводные камни

- **`bind` длина — `sizeof addr`, не выражение.** Ошибка скобки `bind(fd, ..., sizeof(addr) == -1)` молча проходит компиляцию (`sizeof` это `size_t`, выражение всегда `0`), `bind` получит длину 0 → `EINVAL`. Закрывающую скобку `bind()` ставить **до** `== -1`.
- **`memset(&addr, 0, sizeof addr)` обязателен** — `sin_zero[8]` мусор → возможный `EINVAL`.
- **`socklen_t` value-result** — инициализировать перед каждым `accept`.
- **`recv == 0` это EOF**, не "пустой ответ".
- **Partial read/write** — `recv`/`send` могут вернуть меньше чем просили.
- **`SA_RESTART` ломает shutdown** — не ставить для SIGINT/SIGTERM.
- **SIGPIPE по дефолту убивает процесс** — защитить `SIG_IGN` или `MSG_NOSIGNAL`.
- **`EPIPE`/`ECONNRESET` — штатное событие**, не печатать `perror`.
- **Listening fd ≠ accepted fd** — закрывать оба, в правильной scope.
- **`htons` vs `htonl`** — `s` для 16 бит (порт), `l` для 32 (IPv4). Перепутаешь — порт 0 / адрес-зеркало.

## Шпаргалка: когда `break`, когда `perror`, когда `continue`

Самое сложное в TCP-сервере — **разделить ошибки по реакции**. `errno` сам по себе ничего не значит — значение придаёт твоё решение что с этим делать.

| Syscall | errno | Реакция | Почему |
|---------|-------|---------|--------|
| `accept` | `EINTR` | `continue` | Прервало сигналом до handshake, попробовать снова; `while(!stop)` проверится |
| `accept` | прочее (`EMFILE`, `ECONNABORTED`...) | `perror` + `continue` | Listening сокет жив, ждём следующего клиента |
| `recv` | `EINTR` | `continue` | Не успел ничего прочитать, повторить |
| `recv` | `ECONNRESET`, `ETIMEDOUT`, `EBADF`... | `perror` + `break` | Это **соединение** мёртвое; `continue` бессмыслен — закрыть `client_fd`, ждать следующего клиента |
| `recv` | возврат `0` | `break` (без perror) | EOF: peer корректно закрыл write-end. **Не ошибка** |
| `send` | `EPIPE`, `ECONNRESET` | `break` **без `perror`** | Peer ушёл (нормально или с RST). Логировать незачем, забьёт stderr |
| `send` | прочее (`EBADF`, `ENOMEM`...) | `perror` + `break` | Это **уже** баг или системная проблема, нужно знать |
| `connect` (client) | любое | `perror` + `exit` | У клиента нет fallback'а — нет соединения, нет программы |

**Главный принцип**: `perror` нужен когда **ты узнаёшь о баге** (своём или системном), а не когда происходит штатное событие. Peer ушёл — это **не баг твоего кода**.

**Второй принцип**: `continue` vs `break` зависит от **что выжило**:
- listening сокет выжил (`accept` ошибка) — `continue`, ждём следующего клиента.
- client сокет умер (`recv`/`send` ошибка) — `break`, закрыть его, потом `continue` внешнего цикла.

**Третий принцип, легко промахнуться**: проверка `errno` имеет смысл **только после возврата `-1`**. `errno` хранит мусор от предыдущих вызовов. Шаблон: сохранить возврат → проверить `== -1` → **только тогда** смотреть `errno`.

```c
ssize_t s = send(fd, buf, n, 0);
if (s == -1) {
    if (errno == EPIPE || errno == ECONNRESET) break;
    perror("send"); break;
}
```

Если делать `if (errno == EPIPE)` **без** проверки возврата — сработает на ровном месте, потому что `errno` мог остаться `EPIPE` после предыдущей операции которая ни при чём.

## Связанные темы

[[fd-kernel-model]] [[pipes-linux]] [[signals-linux]] [[async-signal-safe]] [[syscalls-linux]] [[proc-net]]
