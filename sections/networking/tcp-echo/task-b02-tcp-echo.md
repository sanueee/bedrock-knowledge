---
task: task-b02
title: TCP echo server + client
block: B1 — TCP-сокеты
date_started: 2026-05-14
date_finished: 2026-05-17
status: выполнено
---

# task-b02 — TCP echo server + client

Первый task блока B. Цель — пройти весь жизненный цикл TCP-соединения на blocking sockets API без надстроек: понять кто что делает из `socket/bind/listen/accept/connect/send/recv/close`, почему сервер пишет в новый fd, что возвращает `accept()`, чем `send/recv` отличается от `write/read`, и где затаились частичные чтения (`partial reads`).

## Перед началом перечитать

- [topics/linux/fd-kernel-model.md](../topics/linux/fd-kernel-model.md) — `socket()` возвращает fd, всё остальное — операции над ним.
- [topics/linux/pipes-linux.md](../topics/linux/pipes-linux.md) — `read()` блокирующая семантика, EOF, частичные чтения. TCP ведёт себя похоже.
- [topics/linux/syscalls-linux.md](../topics/linux/syscalls-linux.md) — `EINTR`/`SA_RESTART`, потому что `accept`/`recv` — медленные syscalls.

Конспекта по сокетам пока нет — создашь через `vault-write` после задачи: `topics/networking/tcp-sockets.md`.

## Задание

Две программы на C: `echo_server` и `echo_client`. Расположение: `networking/echo/` (создать директорию).

### echo_server

1. Принимает один аргумент CLI — порт (uint16, валидировать через `strtol` с проверкой границ).
2. Создаёт TCP-сокет, ставит флаг `SO_REUSEADDR` (объяснить в vault зачем — TIME_WAIT).
3. Биндится на `INADDR_ANY:<port>`.
4. Слушает с `backlog=8`.
5. В цикле:
   - `accept()` нового клиента, печатает IP-адрес и порт клиента в stderr (через `inet_ntop`).
   - В цикле `recv()` → `send()` обратно ровно столько байт, сколько прочитал. Если `recv()` вернул 0 — клиент закрыл соединение, выйти из внутреннего цикла, `close(client_fd)`, ждать следующего.
   - Если `recv()`/`send()` вернул `-1` и `errno == EINTR` — повторить вызов (или установить `SA_RESTART` для SIGINT — на выбор, обосновать).
   - Любая другая ошибка — `perror`, `close(client_fd)`, продолжить цикл `accept`.
6. Обработчик `SIGINT` → флаг `volatile sig_atomic_t stop = 1`, основной цикл выходит, `close(listen_fd)`, печать "bye" в stderr.

В этой версии — **один клиент за раз**, без потоков/fork. Конкурентность — позже (B-расширение или D1/tokio).

### echo_client

1. Принимает два аргумента — `<host>` и `<port>`. Host может быть IPv4 в виде строки ("127.0.0.1") — достаточно `inet_pton`, без DNS на этом этапе.
2. Создаёт сокет, `connect()` на заданный адрес.
3. В цикле читает строки из `stdin` через `fgets()` → `send()` на сервер → `recv()` ответ → печатает в `stdout`. Условие выхода — EOF на stdin (Ctrl-D).
4. `close(sock)` перед выходом.

### Что протестировать

- Запустить сервер на 9000, клиент в другом терминале → отправить несколько строк, проверить echo.
- `Ctrl-C` на сервере → корректное "bye", нет утечки порта (запустить сразу второй раз — должно сесть благодаря `SO_REUSEADDR`).
- Клиент → `Ctrl-D` → сервер видит `recv()==0`, закрывает соединение, готов принять следующего.
- `telnet 127.0.0.1 9000` тоже должен работать — это проверка что протокол не выдуман.
- Запустить два клиента одновременно — второй должен ждать пока первый не отключится (один клиент за раз).

### Подводные камни (заранее)

- `recv(fd, buf, N, 0)` может вернуть **меньше** N байт. Это нормально для stream-сокета. Не считать что прочитал всё за один вызов.
- `send(fd, buf, M, 0)` тоже может записать меньше M. Для маленьких payload'ов почти всегда пишет всё, но правильная реализация — цикл по остатку. На этом task'е можно ограничиться одним вызовом и комментарием "TODO: write_all helper", но запиши это в vault как известное ограничение.
- `struct sockaddr_in.sin_port` и `.sin_addr.s_addr` — в **network byte order** (big-endian). `htons()`/`htonl()`. Это та же тема что и little-endian в `/proc/net/tcp`, но в обратную сторону.
- `bind()` без `SO_REUSEADDR` после `Ctrl-C` сервера даст `EADDRINUSE` пока TIME_WAIT не истечёт. Это и есть демонстрация зачем `SO_REUSEADDR`.

## Хедеры (явный список)

| Зачем | Хедер |
|-------|-------|
| `socket`, `bind`, `listen`, `accept`, `connect`, `send`, `recv`, `setsockopt`, `SOCK_STREAM`, `SOL_SOCKET`, `SO_REUSEADDR` | `<sys/socket.h>` |
| `struct sockaddr_in`, `htons`, `htonl`, `INADDR_ANY`, `IPPROTO_TCP` | `<netinet/in.h>` |
| `inet_pton`, `inet_ntop` | `<arpa/inet.h>` |
| `close` | `<unistd.h>` |
| `errno` | `<errno.h>` |
| `perror`, `fprintf`, `fgets`, `stdin`, `stderr`, `stdout` | `<stdio.h>` |
| `exit`, `EXIT_FAILURE`, `EXIT_SUCCESS`, `strtol` | `<stdlib.h>` |
| `memset`, `strlen` | `<string.h>` |
| `sigaction`, `struct sigaction`, `sig_atomic_t`, `SIGINT` | `<signal.h>` |

## Критерии готовности

- [ ] `echo_server` и `echo_client` компилируются без warning'ов (`gcc -Wall -Wextra -std=c11`).
- [ ] Echo работает между server↔client.
- [ ] `Ctrl-C` на сервере корректен, повторный запуск без задержки.
- [ ] Клиент с EOF чисто отключается, сервер готов принять следующего.
- [ ] Раздел "Vault" в этом файле заполнен через `vault-write` после задачи: новый файл `topics/networking/tcp-sockets.md` + ссылка на него отсюда.

## Vault

- [[tcp-sockets]] — `topics/networking/tcp-sockets.md` (создан 2026-05-15). Покрывает: жизненный цикл сокетов, sockaddr_in/htons/htonl, SO_REUSEADDR/TIME_WAIT, accept value-result, recv три исхода и partial read, EINTR на разных слоях, EPIPE/ECONNRESET как штатное событие, SIGPIPE три способа защиты.

---

## Моё решение (server)

Серверная половина (`echo_server.c`) написана и прошла ревью.

Подход: blocking sockets, один клиент за раз (без потоков/fork), `SIGINT` через `sigaction` без `SA_RESTART` чтобы `accept` прерывался через `EINTR` и `while(!stop)` имел шанс выйти. `SIGPIPE` — `SIG_IGN` отдельной `sigaction`, обработка через `errno == EPIPE` в `send`. Внутренний `recv/send` цикл с явным разбором трёх случаев `recv` (0/-1/>0) и обработкой `EINTR`/`EPIPE`/`ECONNRESET`.

Прошло ~5 итераций `/check`. Главные правки между итерациями:
1. Скобка съела `==` в `bind(fd, addr, sizeof(addr) == -1)` — два раза не замечал.
2. На SIGPIPE сначала поставил **handler с печатью** — пришлось переучиваться: цель `SIGPIPE` это **подавить**, а не ловить.
3. accept-ветка на `EINTR` проваливалась дальше с `client_fd == -1` — добавил `continue`.
4. `inet_ntop` без `fprintf` — мёртвая запись в буфер.
5. `close(client_fd)` забыл — fd-утечка.
6. `"bye.."` стоял внутри внешнего `while` (печать после каждого клиента вместо одной при выходе).

## Ключевые функции (server)

| Функция | Зачем |
|---------|-------|
| `socket(AF_INET, SOCK_STREAM, 0)` | TCP-сокет |
| `setsockopt(SO_REUSEADDR)` | пережить TIME_WAIT при повторном запуске |
| `bind(fd, addr, len)` | привязать к INADDR_ANY:port |
| `listen(fd, 8)` | passive mode, backlog 8 |
| `accept(fd, &caddr, &clen)` | принять клиента, получить новый fd; `clen` — value-result |
| `recv(cfd, buf, n, 0)` | три исхода: 0 (EOF), -1 (errno), >0 |
| `send(cfd, buf, n, 0)` | возможен `-1` с `EPIPE`/`ECONNRESET` |
| `inet_ntop(AF_INET, &caddr.sin_addr, ip, INET_ADDRSTRLEN)` | log клиента — бинарный IP в строку |
| `htons(port)`, `htonl(INADDR_ANY)` | host → network byte order |
| `sigaction(SIGINT, ...)` без `SA_RESTART` | чтобы accept прерывался через EINTR |
| `sigaction(SIGPIPE, &SIG_IGN)` | подавить терминирование при записи в мёртвое соединение |
| `close(client_fd)`, `close(server)` | освободить fd |

## Что узнал

- **Listening fd и accepted fd — два разных сокета.** Закрытие одного не закрывает другой.
- **`recv == 0` — это EOF, peer закрыл write-end.** Не "ничего не пришло". `recv` блокируется когда данных нет; 0 возвращает только после получения FIN.
- **Partial read** на stream-сокете: `recv(buf, N)` может вернуть **любое** число от 1 до N. Один `send` ≠ один `recv`. TCP — поток байтов, не сообщения. Если нужны границы — счётчик в протоколе.
- **`SA_RESTART` отключает `EINTR`** — ядро автоматически перезапустит syscall. Полезно для устойчивости, но **смертельно для shutdown**: `accept` никогда не вернётся, `while(!stop)` не выполнится, Ctrl-C игнорируется.
- **`SIGPIPE` приходит отправителю**, не получателю. Если игнорировать `recv == 0` и продолжать `send` — второй или третий `send` положит сервер.
- **`SIG_IGN` для SIGPIPE — это подавление**, не "поймать сигнал". Не нужно ставить handler — нужно сказать ядру "не доставляй".
- **`EPIPE` и `ECONNRESET` — штатные события**, без `perror`. Это значит peer ушёл, переходим к следующему клиенту.
- **`socklen_t` — value-result параметр.** Инициализировать `sizeof` **перед каждым** `accept`/`recvfrom`. С `sockaddr_in` стабильно (всегда 16), но с `sockaddr_storage` (универсальный v4/v6) — без инициализации усечётся IPv6.
- **`TIME_WAIT` ~60s** держит порт после закрытия. Без `SO_REUSEADDR` — `EADDRINUSE` при повторном `bind`.
- **`htons` (16 бит, порт) vs `htonl` (32 бита, IPv4).** Network byte order = big-endian. Зеркально к `/proc/net/tcp` (task-a07).
- **`memset(&addr, 0, sizeof addr)` обязателен** — есть `sin_zero[8]` padding, мусор → `EINVAL`.
- **`strtol` валидация по трём условиям**: `errno != 0` (overflow ERANGE), `end == argv[1]` (не съел ни символа = не число), `*end != '\0'` (хвост после числа). errno **это не битовая маска**, обычный thread-local int.

## Ошибки и трудности

- Дважды пропустил скобочный баг в `bind` (`sizeof(addr) == -1` внутри вызова). Компилируется молча, длина приходит как `0`, `bind → EINVAL`. Урок: на ошибку `bind` ставить `perror("bind")`, а не свой `fprintf` — увидел бы `Invalid argument` сразу.
- Концептуальное непонимание про SIGPIPE: думал что надо "поймать" — пришлось переучить на "подавить через SIG_IGN".
- В контрольных вопросах:
  - **SA_RESTART** объяснил неверно (думал "не даст убить процесс"). На самом деле — auto-restart прерванного syscall'а, что ломает выход через `EINTR`.
  - **`recv == 0`** считал "пустым ответом" — это EOF.
  - **Partial read**: думал что `recv(buf, 4096, 0)` при 10000 байт вернёт `-1`. Возвращает ≤ 4096, остальное в kernel-буфере.
  - **`EPIPE`/`ECONNRESET`** — не знал что это штатные события и почему без `perror`.
  - **`errno`** назвал битовой маской — это обычный `int` (thread-local).
  - **`socklen_t` value-result** — объяснил как "будет вечно стучаться к мёртвому клиенту", это совсем не то. Реальная проблема — усечение sockaddr_storage при IPv6.
  - **8-я проверка `strtol`**: `end == argv[1]` интерпретировал наоборот ("первые символы совпали") — это **наоборот**, проверка ловит "не съели ни одного символа = не число".

## Что бы сделал иначе

- `perror` на каждой системной ошибке с самого начала (особенно `bind`) — сэкономило бы 2 итерации.
- Сразу пройти контрольные вопросы по теории до начала кода — partial read и `recv == 0` я бы написал заранее правильно в комментариях, а не учил по факту.
- Минимальный `SIGINT`-handler (только `stop = 1`, без `write`) — async-signal-safety идиома, печать "bye" один раз в main.

## Самое сложное (из session-debrief)

> Самое сложное это флаги и сигналы которые защищают от тех или иных сценариев, которые могут возникнуть в tcp соединении сервера и клиента — поначалу очень тяжело было понять для чего какая проверка, где на errno нужен просто break а где perror.

Это самый ценный итог сессии — не сами syscall'ы (они стандартные), а **дисциплина разделения ошибок по реакции**. Зафиксировано в [[tcp-sockets]] разделом "Шпаргалка: когда break, когда perror, когда continue".

Главные правила, которые из этого выкристаллизовались:
1. `perror` пишется когда **ты узнаёшь о баге** (своём или системном). Уход peer'а — не баг.
2. `continue` vs `break` зависит от того **что выжило**: listening fd выжил → `continue`, client fd умер → `break`.
3. `errno` смотреть **только** после `== -1`. Иначе читаешь мусор от предыдущих вызовов.

## Финальное тестирование (end-to-end)

Прошли 2026-05-17:
- ✅ Базовый echo через свой клиент: hello/testin/!/hi/lll.
- ✅ `Ctrl-C` на сервере → корректное "bye..", сразу повторный запуск работает (доказательство что `SO_REUSEADDR` отрабатывает).
- ✅ Три клиента подряд (порты 36692, 43102, 58046) — `accept` возвращался следующему после каждого Ctrl-D, listening сокет жил.
- ✅ `telnet 127.0.0.1 9000` подключился, символы эхом возвращались (доказательство что протокол стандартный TCP, не выдуман).
- `telnet` в character mode шлёт каждый байт отдельно — для следующих тестов лучше `nc` (line-mode по умолчанию).

## Код

- `sections/networking/tcp-echo/echo_server.c` — готов.
- `sections/networking/tcp-echo/echo_client.c` — готов.
- Бинарники: `server_echo`, `client_echo`.

## Связанные темы

[[tcp-sockets]] [[signals-linux]] [[async-signal-safe]] [[fd-kernel-model]] [[pipes-linux]] [[proc-net]]
