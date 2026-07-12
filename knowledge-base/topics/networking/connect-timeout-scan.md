# Connect-with-timeout — примитив connect-scan'а

Задача B-8 (`sections/networking/connect-timeout/connect_timeout.c`). Синтез [[nonblocking-poll]] (B-6, `EINPROGRESS`/`SO_ERROR`) + [[event-loop-epoll]] (B-7, `epoll`/`EPOLLOUT`). Ядро TCP connect scan'а: одна проба одного порта с управляемым таймаутом, без блокировки и без root.

## Проблема

Blocking `connect` на закрытый/фильтрованный порт висит на дефолтном таймауте ядра (десятки секунд, `net.ipv4.tcp_syn_retries`). Для сканера это неприемлемо: нужен **свой** таймаут и не блокировать поток. Решение — non-blocking `connect` + ждать готовности через `epoll` с таймаутом.

## Механика (3 шага)

1. **non-blocking `connect`** → `-1` + `errno == EINPROGRESS` (handshake поехал в фоне; см. [[nonblocking-poll]]). Возможен синхронный исход `0` (мгновенный успех, loopback) или `-1` с другим `errno` (жёсткая ошибка — `ENETUNREACH` и т.п.).
2. **`epoll` ждёт `EPOLLOUT`** с таймаутом: `epoll_wait(epfd, &out, 1, timeout_ms)`. Вырожденный случай event loop'а из B-7 — один fd, `maxevents=1`, без `while(1)`. Возврат: `0` → таймаут, `>0` → готов, `-1`+`EINTR` → повторить.
3. **`getsockopt(SO_ERROR)`** — снять отложенную ошибку соединения. **Это единственный источник вердикта**, `EPOLLOUT` сам по себе — нет.

## Три исхода и как различить

| Исход | Как определяется |
|-------|------------------|
| **open** | `EPOLLOUT` → `SO_ERROR == 0` (или синхронный `connect()==0`) |
| **closed** | `EPOLLOUT` → `SO_ERROR == ECONNREFUSED` (хост ответил RST) |
| **filtered** | `epoll_wait` вернул `0` (таймаут — ответа нет вообще, файрвол дропает) |

## Ключевой урок: readiness ≠ correctness (рецидив из B-6)

Сокет становится writable (`EPOLLOUT`) **и при успехе, и при отказе** — RST тоже делает fd готовым к записи. Поверить `EPOLLOUT` = classify закрытый порт как открытый → сканер врёт про поверхность атаки. Различает **только** `getsockopt(SO_ERROR)`. Тот же принцип, что `POLLOUT` в B-6, только через epoll.

## Синхронный vs асинхронный refused

Отказ (`ECONNREFUSED`) приходит по **двум** каналам:
- **асинхронно** — обычный путь: `connect`→`EINPROGRESS`, потом `SO_ERROR==ECONNREFUSED` после `EPOLLOUT`. Так ответил loopback в тесте под Docker/Linux.
- **синхронно** — на loopback ядро иногда знает об отсутствии слушателя сразу: `connect()` возвращает `-1` с `errno==ECONNREFUSED` **немедленно**, не через `EINPROGRESS`. Наивный код (`errno != EINPROGRESS → ERROR`) тогда покажет `error` вместо `closed`. Для полноты сканеру стоит ловить синхронный `ECONNREFUSED` в ветке самого `connect()`. (В прогоне не выстрелило — здесь refused пришёл асинхронно.)

## Security / надёжность

- **fd-leak = потолок сканера.** Сокет открывается на каждый порт; забытый `close` на ветке ошибки/таймаута → упор в `RLIMIT_NOFILE` (`EMFILE`) через сотню портов. `close(fd)` **и** `close(epfd)` обязательны на **всех** путях выхода (early return тоже). Тот же класс, что fd-leak в ревью B-7.
- **`EINTR` на `epoll_wait`** не перезапустить → сигнал превратится в ложный `filtered`. Штатное событие: `while((n=epoll_wait(...))==-1 && errno==EINTR);` — пустое тело крутит ретраи, разбор `n` идёт **после** цикла.
- **Валидация недоверенного ввода:** порт вне `1..65535`, мусорный timeout отсечь до пробы (иначе `int`-усечение `70000`→4464 и `htons` от мусора).

## Тестовое окружение врёт (Docker Desktop на macOS)

`filtered` почти не воспроизвести под Docker Desktop: исходящий трафик контейнера идёт через userspace NAT-прокси (gVisor/vpnkit/gvproxy) в лёгкой VM. Прокси **сам завершает TCP-handshake локально** (отвечает SYN-ACK раньше, чем узнаёт про недоступность upstream) → `connect()` изнутри контейнера успешен, `SO_ERROR==0`, любой адрес выглядит `open`. Код не врёт — врёт стек. Честный `filtered` ловится в обход прокси на L2: несуществующий **on-link** адрес в подсети контейнера (`172.17.255.254`) — ARP уходит в никуда, `epoll_wait` истекает → `filtered`. Разные containers = разные network namespace = разные `127.0.0.1` (слушатель и пробер должны быть в одном контейнере, сервер в фон через `&`).

## Ключевые термины (English)

- **non-blocking connect** — `connect()` на `O_NONBLOCK`-сокете, возврат сразу
- **`EINPROGRESS`** — handshake начат, идёт в фоне
- **connect-with-timeout** — паттерн: non-blocking connect + `epoll`/`poll` с таймаутом
- **`EPOLLOUT` readiness** — сокет готов к записи (готовность ≠ успех)
- **`SO_ERROR`** — pending socket error, read-and-clear через `getsockopt`
- **filtered vs closed vs open** — три исхода пробы порта
- **fd exhaustion** — `EMFILE` при упоре в `RLIMIT_NOFILE`
- **userspace NAT proxy** (vpnkit/gvproxy) — Docker Desktop network stack, ACK'ает всё
