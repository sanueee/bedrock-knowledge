---
type: integration
block-position: B-5
status: выполнено
created: 2026-06-06
completed: 2026-06-06
code: self-practice/networking/host-probe/host_probe.c
synthesizes: [B-2 sockets, B-3 layers, B-4 resolve]
tags: [networking, sockets, getaddrinfo, banner-grab, integration]
---

# B-5 (integration) — `host-probe`

Резолв имени → TCP connect по списку адресов → recv баннера сервиса → печать.
**Без новой теории** — совмещение B-2 (sockets), B-3 (понимание слоёв), B-4 (resolve).

---

## Задание

Написать утилиту `host_probe`, которая по `host` и `port` устанавливает TCP-соединение
и читает «баннер» — то, что сервер присылает первым (SSH, SMTP, FTP — присылают строку
приветствия сразу после connect).

### Запуск

```
./host_probe <host> <port>
```

Примеры для проверки:
- `./host_probe github.com 22` — должен прийти SSH-баннер (`SSH-2.0-...`).
- `./host_probe smtp.yandex.ru 25` — SMTP greeting (`220 ...`), если порт не зарезан провайдером.
- `./host_probe example.com 80` — HTTP-сервер баннер первым **не** шлёт → должен корректно отработать таймаут/пустой ответ (см. ниже), без вечного зависания.

### Что должна делать программа (пайплайн)

1. **Resolve (B-4).**
   - `getaddrinfo(host, port, &hints, &res)`. `port` передаётся как **service-строка** (вторым аргументом `argv[2]` напрямую — getaddrinfo сам разберёт и `"22"`, и `"ssh"`).
   - `hints`: `memset` в ноль, затем `ai_family = AF_UNSPEC` (и IPv4, и IPv6), `ai_socktype = SOCK_STREAM`.
   - При ошибке — `gai_strerror(rc)` (свой namespace, **не** `errno`/`perror`).

2. **Connect по списку (B-2 + B-4, паттерн Redis `_anetTcpGenericConnect`).**
   - Итерация по связному списку: `for (p = res; p != NULL; p = p->ai_next)`.
   - На каждом узле: `socket(p->ai_family, p->ai_socktype, p->ai_protocol)` → при `-1` `continue`.
   - `connect(fd, p->ai_addr, p->ai_addrlen)` → при успехе `break` (нашли рабочий адрес), при неудаче `close(fd)` и пробуем следующий.
   - Если список кончился без успеха — сообщить «не удалось подключиться» и выйти с ненулевым кодом.
   - **Перед connect** (или сразу после, до итерации) — напечатать, к какому IP коннектимся: достать его из `p->ai_addr` кастом по `p->ai_family` → `sockaddr_in`/`sockaddr_in6`, `inet_ntop` + `ntohs(port)`. Это прямой перенос навыка из B-4.

3. **Banner grab (B-2, three-way recv).**
   - `recv(fd, buf, sizeof(buf)-1, 0)` один раз (одного чтения для баннера достаточно).
   - Обработать **все три исхода**: `> 0` (есть данные — завершить строку `\0`, напечатать), `0` (peer закрыл соединение — «соединение закрыто без баннера»), `-1` (ошибка — `perror`, но `EINTR` обработать через retry).

4. **Таймаут на чтение (иначе `example.com:80` повиснет навсегда).**
   - Поставить `SO_RCVTIMEO` на сокет **до** `recv` через `setsockopt`:
     ```c
     struct timeval tv = { .tv_sec = 3, .tv_usec = 0 };
     setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
     ```
   - Тогда `recv` на молчащем сервере вернёт `-1` с `errno == EAGAIN` (или `EWOULDBLOCK`) через 3 секунды. Этот случай обработать отдельно: «сервер не прислал баннер (таймаут)».

5. **Cleanup.**
   - `freeaddrinfo(res)` всегда после работы со списком.
   - `close(fd)`.

### Обязательно

- Защита от `SIGPIPE` не критична (мы только читаем), но `EINTR` на `recv` — обработать (retry в цикле или повторный вызов). Подумай, почему `SO_RCVTIMEO` + `EINTR` — это два разных `-1`/`errno`.
- Проверка `argc == 3`, иначе usage в `stderr` и выход.
- Никакого парсинга Ethernet/IP-заголовков (это L7-поверх-готового-TCP — связь с B-3 концептуальная: ты читаешь payload, ядро уже сняло L2/L3/L4).

### Хедеры (соответствие функция → include)

| Функция / тип | Header |
|---|---|
| `getaddrinfo`, `freeaddrinfo`, `gai_strerror`, `struct addrinfo`, `AF_UNSPEC`, `SOCK_STREAM` | `<sys/types.h>`, `<sys/socket.h>`, `<netdb.h>` |
| `socket`, `connect`, `recv`, `setsockopt`, `SO_RCVTIMEO`, `SOL_SOCKET` | `<sys/socket.h>` |
| `struct sockaddr_in`, `sockaddr_in6`, `inet_ntop`, `ntohs` | `<netinet/in.h>`, `<arpa/inet.h>` |
| `struct timeval` | `<sys/time.h>` |
| `close` | `<unistd.h>` |
| `errno`, `EAGAIN`, `EWOULDBLOCK`, `EINTR` | `<errno.h>` |
| `perror`, `fprintf`, `snprintf` | `<stdio.h>` |
| `memset`, `strlen` | `<string.h>` |
| `exit`, `EXIT_FAILURE` | `<stdlib.h>` |

### Компиляция

```
cc -Wall -Wextra -o host_probe host_probe.c
```

(getaddrinfo в glibc — без доп. библиотек, `-lpcap` тут не нужен.)

### Код

`self-practice/networking/host-probe/host_probe.c`

---

## Контрольные вопросы (ответить при разборе)

1. Почему `getaddrinfo` возвращает **список**, а не один адрес? Зачем итерировать с попыткой connect, а не брать `res` напрямую?
2. `recv` вернул `-1`. Как по `errno` отличить *таймаут* (`SO_RCVTIMEO` сработал) от *прерывания сигналом* (`EINTR`) — и почему реакция на них разная?
3. `recv` вернул `0`. Что это значит на уровне TCP (какой сегмент пришёл) и почему это **не** ошибка?
4. Почему HTTP-сервер на :80 баннер первым не шлёт, а SSH/SMTP — шлют? Что это говорит о том, кто «говорит первым» в протоколе?
5. Каст `p->ai_addr` к `sockaddr_in *` — он двигает байты или меняет схему чтения? (повтор B-4, проверка что закрепилось)

---

## Разбор

### Моё решение

Написал за 3 итерации `/check`. Пайплайн как в задании: `getaddrinfo` (AF_UNSPEC + SOCK_STREAM) → `for (p = info; p; p = p->ai_next)` с `socket`→`connect`→`break`, неудача → `close`+`continue` → после break печать IP кастом по `ai_family` (`inet_ntop` + `ntohs`) → `SO_RCVTIMEO` 3 сек → `do/while` recv с retry на EINTR → разбор трёх исходов (`>0`/`0`/`-1` с EAGAIN vs perror) → `freeaddrinfo` + `close`. Собралось чисто с `-Wall -Wextra`.

### Что узнал

- **`SO_RCVTIMEO`** — read timeout на сокете, иначе banner grab по HTTP-порту висит навсегда (сервер ждёт запрос, клиент ждёт баннер — клинч). По истечении → `recv` возвращает `-1`/`EAGAIN`.
- **`EAGAIN`(таймаут) vs `EINTR`(сигнал)** — два разных `-1` с противоположной реакцией: EINTR → retry, EAGAIN → принять как «баннера нет». Идиома: «съесть» EINTR в `do/while` до разбора веток.
- **Перебор адресов** `getaddrinfo` с попыткой connect — паттерн «первый рабочий выигрывает», как `_anetTcpGenericConnect` в Redis. Две точки отказа (socket/connect), одна успеха.
- **lifetime + `freeaddrinfo`**: нельзя обращаться к `p` (в т.ч. `p == NULL`) после `free` списка — указатель смотрит в освобождённую память (перенёс `freeaddrinfo` ниже проверки). Прямая параллель с Rust-ownership, который идёт параллельно.
- **Буферизация stdout/stderr** (всплыло при тесте): `connected:` (stdout, fully buffered под pipe) напечаталось после `timeout` (stderr, unbuffered). Артефакт перенаправления, не код. → [[stdio-buffering]].

### Ошибки и трудности

Содержательно всё понятно (слова пользователя: «вероятно синтаксис ещё не до идеала отточил, а так всё понятно»). Шероховатости были именно в коде, не в концепциях:
- В первой версии забыл `freeaddrinfo` (утечка) и `close(fd)` в cleanup.
- Не добавил печать IP — это и есть смысл интеграции B-4, пришлось дописывать.
- Во второй версии `freeaddrinfo` встал до проверки `p == NULL` → обращение к dangling-указателю (поправил в третьей).
- Остаточная мелочь (осознанно оставлена): на `inet_ntop == NULL` всё равно печатается `buf`.

### Что бы сделал иначе

Сразу держать в голове чек-лист cleanup (free + close) и не отделять «печать IP» от connect — это часть успешной ветки, а не довесок.

### Тесты (вживую)

- `github.com 22` → `connected: 140.82.121.3:22` + `banner: SSH-2.0-...` ✅
- `example.com 80` → `server didn't send banner (timeout)` через ~3с ✅ (не зависло)
- `nosuchhost.invalid 22` → `nodename nor servname provided` через `gai_strerror`, exit≠0 ✅

## Связи

- [[getaddrinfo]] — резолв, addrinfo linked list, каст по ai_family
- [[tcp-sockets]] — socket/connect/recv, three-way recv, partial read
- [[redis-anet]] — паттерн итерации по адресам с connect (`_anetTcpGenericConnect`)
- [[ethernet-frame]] — L2/L3/L4, поверх которых лежит читаемый баннер (концептуальная связь)
- [[stdio-buffering]] — почему `connected:` (stdout) печатается после `timeout` (stderr) под pipe
