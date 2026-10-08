---
тема: Redis anet.c — обёртка над sockets API
блок: Reading
дата: 2026-05-20
связано:
  - "[[tcp-sockets]]"
  - "[[event-loop-epoll]]"
  - "[[nagle-tcp-nodelay]]"
  - "[[getaddrinfo]]"
  - "[[bitwise-flags]]"
---

# Redis anet.c — обёртка над sockets API

## Что это

`redis/src/anet.c` — слой Redis, который оборачивает голый POSIX-сокеты API (`socket`/`bind`/`listen`/`accept`/`connect`) в безопасные функции `anetTcpServer`/`anetTcpAccept`/`anetTcpGenericConnect`. Цель — скрыть от вышестоящего кода: ошибки шагов, выбор IPv4/IPv6, гигиену сокета (SO_REUSEADDR, O_NONBLOCK, TCP_NODELAY), cleanup на полупути.

## Архитектурные выводы

### 1. Библиотека не печатает и не умирает

Своё приложение: `perror("bind"); exit(1);`. Библиотека так не имеет права — она не знает контекста (это сервер? клиент? тест?). Паттерн в `anet.c`:
```c
anetSetError(err, "bind: %s", strerror(errno));
return ANET_ERR;
```
- `err` — буфер `char err[ANET_ERR_LEN]`, **передаётся вызывающим**.
- `anetSetError` пишет в него человекочитаемое сообщение через `vsnprintf`.
- Возвращается код, вызывающий сам решает: лог, повтор, выход.

Это базовая дисциплина любого библиотечного кода.

### 2. Симметрия connect и server: одна и та же структура

Обе функции (`anetTcpGenericConnect`, `_anetTcpServer`) построены одинаково:
```
getaddrinfo → for each candidate (p):
                socket() → continue if fail
                setsockopt (SO_REUSEADDR, и т.д.)
                bind / connect
                break / continue on success/fail
            cleanup on full failure (p == NULL)
            freeaddrinfo(servinfo)
```

Это паттерн **"resolve → попробовать кандидатов по очереди"**. Связан с тем, что [[getaddrinfo]] возвращает список — нужно итерироваться, потому что первый адрес может не сработать (IPv6 на v4-only хосте, например).

### 3. Управление fd при ошибке "посередине"

Сценарий: `socket()` ок, `connect()` упал. Кто закроет fd? Два разных приёма в файле:

**В `anetTcpGenericConnect`** — `goto error` + общий cleanup в конце:
```c
if (anetSetReuseAddr(err, s) == ANET_ERR) goto error;
...
error:
    if (s != ANET_ERR) { close(s); s = ANET_ERR; }
end:
    freeaddrinfo(servinfo);
```
Метки `error:` и `end:` гарантируют: одно место закрытия fd, одно место `freeaddrinfo`. Классическая идиома "single cleanup path" в C — заменяет отсутствующий try/finally.

**В `anetCreateSocket`** — ранний return с inline close:
```c
if (anetSetReuseAddr(err, s) == ANET_ERR) {
    close(s);                     // закрываем здесь же
    return ANET_ERR;
}
```
Работает когда **одна точка возможной ошибки**. Когда их много — `goto error` чище.

### 4. Гигиена сокета по умолчанию

Что Redis выставляет на каждом сокете без вопросов:
- **`SO_REUSEADDR`** — чтобы можно было перезапустить сервер сразу, не ждать TIME_WAIT.
- **`O_NONBLOCK`** — обязательно для event loop модели (см. [[event-loop-epoll]]). Без него один медленный клиент остановит всех.
- **`TCP_NODELAY`** на accepted сокетах — Redis это request-response с микросекундной латентностью, Нагл недопустим (см. [[nagle-tcp-nodelay]]).
- **`FD_CLOEXEC`** на новых fd — не давать форкнутым дочерним процессам наследовать сокет (через `accept4` + `SOCK_CLOEXEC` или вручную через `anetCloexec`).

В своём `echo_server.c` ничего из этого, кроме `SO_REUSEADDR`, не было. Это типичный gap "учебный код vs production".

### 5. Best-effort binding — graceful degradation

```c
if (s == ANET_ERR && source_addr && (flags & ANET_CONNECT_BE_BINDING)) {
    return anetTcpGenericConnect(err, addr, port, NULL, flags);
}
```
Если хотели привязаться к конкретному source-адресу, но не смогли — попробовать ещё раз **без него**. Лучше подключиться хоть как-то, чем сорвать операцию. Применяется в кластерных сценариях (Redis cluster bus, replication) где source-bind — рекомендация, не требование.

### 6. Async connect + SO_ERROR

```c
if (connect(s, p->ai_addr, p->ai_addrlen) == -1) {
    if (errno == EINPROGRESS && flags & ANET_CONNECT_NONBLOCK)
        goto end;          // НЕ ошибка, handshake в фоне
    close(s); s = ANET_ERR; continue;
}
```
Для неблокирующего connect `EINPROGRESS` — штатный путь. Вызывающий потом ждёт через epoll `EPOLLOUT` и проверяет результат через `getsockopt(SO_ERROR)` (функция `anetGetError`). См. [[event-loop-epoll]] раздел "Readiness ≠ correctness".

## Интересные приёмы

- **Свой namespace ошибок + своя strerror** (детально в [[getaddrinfo]]): `getaddrinfo` не выставляет `errno`, возвращает свой код, расшифровка через `gai_strerror`. Это потому что многошаговая операция (открыть hosts, DNS-запрос, парсинг) — `errno` отражает причину последней операции, не главной. Паттерн встретится в OpenSSL, libcurl, zlib.
- **Условный `accept4`**: `#ifdef HAVE_ACCEPT4` — на Linux одним вызовом `accept4(SOCK_NONBLOCK|SOCK_CLOEXEC)` атомарно. На системах без — два дополнительных `fcntl`. Без атомарности есть race: между accept и fcntl может произойти fork, дочерний процесс унаследует "грязный" fd.
- **`AI_PASSIVE` + `bindaddr == NULL` → wildcard**: Redis: если `bindaddr` == `"*"` или `"::*"`, обнуляет до NULL, и getaddrinfo с `AI_PASSIVE` подставляет `INADDR_ANY`. Кодирование "слушать на всех интерфейсах" через "отсутствие host".
- **Идиома `!!(flags & X) == !!param`** для нормализации сравнения bit-флага с bool-параметром. См. [[bitwise-flags]].

## Что осталось непонятным / открытые вопросы

- **`anetV6Only` (`IPV6_V6ONLY`)** — почему на IPv6-сокете надо явно сказать "только v6"? По умолчанию IPv6-сокет на Linux принимает и v4-маппинг (`::ffff:1.2.3.4`). Это иногда нежелательно (двойной listen на одном порту v4 и v6 даст конфликт). Стоит разобрать отдельно когда дойдём до dual-stack.
- **`anetPipe` с `pipe2` fallback** — кросс-платформенный паттерн для pipe + флаги, но логика бранчей с pipe_flags странная — стоит вернуться после задачи по pipe2/cloexec.
- **`anetAcceptFailureNeedsRetry`** — список errno, при которых accept-loop НЕ должен сдаваться (`ECONNABORTED`, `ENETDOWN`, ...). Это база для robust accept loop, но детально каждый код не разобран.

## Связанные темы

[[tcp-sockets]] [[event-loop-epoll]] [[nagle-tcp-nodelay]] [[getaddrinfo]] [[bitwise-flags]] [[fd-kernel-model]]
