---
тема: Блокирующие vs неблокирующие сокеты + poll / Blocking vs non-blocking sockets + poll
блок: B — Сетевой стек
дата: 2026-06-07
связано:
  - "[[tcp-sockets]]"
  - "[[event-loop-epoll]]"
  - "[[getaddrinfo]]"
  - "[[bitwise-flags]]"
---

# Блокирующие vs неблокирующие сокеты + poll / Blocking vs non-blocking sockets + poll

> Эта запись — прицельно по слабому месту (отмечено в B-6): тема всплывает 3-й раз (B-2, B-5, B-6), а автоматизма нет. Цель — один раз разложить «кто ждёт, кто читает, кто ограничивает время».

## Что это

**Режим сокета (socket mode)** — решает, что делает syscall (`connect`/`recv`/`send`/`accept`), когда **операция не может завершиться прямо сейчас** (нет данных, handshake не доехал, буфер полон).

- **blocking (блокирующий)** — режим по умолчанию: syscall **усыпляет поток**, пока не сможет выполниться (или вечно).
- **non-blocking (неблокирующий)** — `O_NONBLOCK`: syscall **никогда не спит**, сразу возвращает `-1` + `errno == EAGAIN` («сейчас нельзя, зайди позже»).

Ключевая мысль: **режим — это про то, кто ждёт.** В blocking ждёт ядро внутри syscall. В non-blocking ждать должен ты сам — и инструмент ожидания это `poll`/`select`/`epoll`.

## Ключевые термины (English)

- **blocking mode** — syscall спит до завершения операции.
- **non-blocking mode** (`O_NONBLOCK`) — syscall возвращается немедленно; «не готово» = `-1` + `EAGAIN`.
- **`EAGAIN` / `EWOULDBLOCK`** — синонимы (один код на Linux/macOS): «операция бы заблокировалась, но режим non-blocking — повтори позже».
- **`EINPROGRESS`** — non-blocking `connect`: handshake **запущен в фоне**, ещё не завершён. Не ошибка.
- **readiness (готовность)** — `poll` сообщает «fd готов к операции» (`POLLIN` читать / `POLLOUT` писать). Готовность ≠ корректность.
- **`SO_ERROR`** — припаркованная на сокете ошибка асинхронной операции (read-and-clear).
- **timeout** — аргумент `poll` (мс): `-1` ждать вечно, `0` опрос без ожидания, `>0` ждать не дольше.

## Как работает: три роли, которые путаются

При non-blocking I/O ожидание разбито на **три отдельные роли**. Туман возникает, когда их сваливают в одну.

| Кто | Роль | Чем НЕ является |
|---|---|---|
| non-blocking режим | syscall не спит, «не готово» = мгновенный `EAGAIN` | не ждёт сам |
| `poll(...)` | **где** поток ждёт готовности fd | не читает данные, не гарантирует успех |
| аргумент `timeout` у `poll` | **сколько** максимум ждать | не свойство `poll` как такового |
| `recv`-цикл | **чем** вычерпываешь пришедшее (до `EAGAIN`/`0`) | не ждёт появления данных |

Мнемоника: **режим = «не спать», `poll` = «где ждать», `timeout` = «сколько ждать», `recv` = «чем читать».**

## Матрица 2×2 — что будет, когда данных/соединения нет

Главный инструмент против тумана. Строки — режим сокета, выбор `poll` — есть/нет.

| Сокет | poll? | `recv`/`connect`, когда «не готово» | Итог |
|---|---|---|---|
| blocking | нет | **висит вечно** | hang (беда B-5 без `SO_RCVTIMEO`) |
| blocking | да (timeout) | `poll` спит до готовности или таймаута | работает, но 1 fd = 1 занятый поток |
| **non-blocking** | **нет** | `EAGAIN` мгновенно → код сдаётся | **данные/баннер потеряны (НЕ hang)** |
| **non-blocking** | да (timeout) | `poll` ждёт → потом `recv` читает | ✓ масштабируемый путь (дорога к epoll) |

Из неё сразу читаются две частые ошибки:
- «Уберу `poll` — программа зависнет» — **неверно** для non-blocking (строка 3): она не зависнет, а **сдастся мгновенно**. Зависание — свойство blocking (строка 1).
- «`poll` спасает от зависания» — спасает **таймаут**, переданный в `poll`. `poll(..., -1)` на молчащем сокете висит вечно сам.

## Различитель режима — elapsed, а НЕ errno (закреплено sockmode_demo, B-interview03)

Слабое место, всплывавшее дважды: «вернул `EAGAIN` → значит non-blocking». **Неверно.** `EAGAIN` означает «операция бы заблокировалась / время вышло — попробуй позже», и его возвращают **оба** режима:

| Конфиг (молчащий сервер) | Режим | elapsed | errno |
|---|---|---|---|
| blocking + `SO_RCVTIMEO`=1с | **blocking** | ~1001 мс | `EAGAIN` |
| non-blocking (`O_NONBLOCK`) | **non-blocking** | ~0 мс | `EAGAIN` |
| blocking + `alarm(2)` | **blocking** | ~2005 мс | `EINTR` |

Первые две строки по `errno` **неотличимы** — один и тот же `EAGAIN`. Различает их только **elapsed**: сокет **спал** (≈ длительность таймаута) → blocking; **не спал** (≈ 0 мс) → non-blocking. Отсюда правило: **режим читается по времени, а не по коду ошибки.**

Следствия:
- `SO_RCVTIMEO` **не меняет режим** — сокет остаётся blocking, таймаут лишь обрезает сон сверху. На non-blocking он бессмыслен: обрезать нечего, `recv` и так не спит.
- `EINTR` (строка 3) приходит от **сигнала**, не «сам по себе». Без `alarm` blocking-`recv` висел бы вечно.
- Иерархия: **режим** = ждать или нет; **таймаут** = сколько ждать, но только когда кто-то ждёт (blocking-сон или `poll`).

## Non-blocking connect — особый случай

`connect` на non-blocking сокете — «fire-and-forget»: ядро шлёт SYN и сразу возвращает `-1` + `EINPROGRESS`. Завершение ловится не повторным `connect` (это даст `EALREADY`), а так:

1. `connect` → `-1`/`EINPROGRESS` (или `0`, если мгновенно — на loopback).
2. `poll(POLLOUT, timeout)` — ждать, пока сокет станет writable (= handshake разрешился) или истечёт таймаут.
3. `getsockopt(SOL_SOCKET, SO_ERROR, &so_err)` — **обязательно**: `POLLOUT` срабатывает и при успехе, и при провале (RST → `ECONNREFUSED`). `so_err == 0` → подключились; иначе → ошибка. Это и есть **readiness ≠ correctness**.

Почему нельзя через `errno`: провал случается **асинхронно**, когда ни один syscall не активен → `errno` (синхронный канал возврата syscall) пуст. Ядро паркует ошибку на сокете → `SO_ERROR` (read-and-clear «почтовый ящик»).

## Пример (из nbconnect.c, B-6)

Перевод в non-blocking — идиома bitwise-флагов (см. [[bitwise-flags]]):
```c
int flags = fcntl(fd, F_GETFL, 0);        // прочитать текущие
fcntl(fd, F_SETFL, flags | O_NONBLOCK);   // добавить бит, не затерев остальные
```

Фаза 1 — добыть подключённый fd:
```c
int res = connect(fd, p->ai_addr, p->ai_addrlen);
if (res == 0) { found = 1; break; }                 // мгновенно
else if (errno == EINPROGRESS) {
    struct pollfd pfd = { .fd = fd, .events = POLLOUT };
    do { n = poll(&pfd, 1, timeout_ms); } while (n == -1 && errno == EINTR);
    if (n == 0) { close(fd); continue; }            // таймаут
    int so_err; socklen_t len = sizeof(so_err);
    getsockopt(fd, SOL_SOCKET, SO_ERROR, &so_err, &len);
    if (so_err == 0) { found = 1; break; }          // подключились
    else { close(fd); continue; }                   // ECONNREFUSED и т.п.
}
else { close(fd); continue; }                       // прочая ошибка
```

Фаза 2 — на подключённом fd прочитать баннер:
```c
struct pollfd pfd = { .fd = fd, .events = POLLIN };
do { n = poll(&pfd, 1, timeout_ms); } while (n == -1 && errno == EINTR);  // ЖДЁМ данные
for (;;) {                                                                // ВЫЧЕРПЫВАЕМ
    ssize_t r = recv(fd, buf, sizeof(buf) - 1, 0);
    if (r > 0)               { buf[r] = '\0'; fputs(buf, stdout); }
    else if (r == 0)          break;            // EOF
    else if (errno == EINTR)  continue;
    else if (errno == EAGAIN) break;            // данных больше нет сейчас
    else { perror("recv"); break; }
}
close(fd);
```

## Подводные камни

- **`EINPROGRESS` обработан как `EINTR`** → `connect` в `do/while` зацикливается, на 2-й итерации `EALREADY`, адрес выбрасывается. `EINPROGRESS` — НЕ повод повторять.
- **Пропустить `getsockopt(SO_ERROR)`** после `POLLOUT` → проваленный connect (`ECONNREFUSED`) принят за успех.
- **Один цикл делает две работы** (поиск адреса + чтение) → ветки `break`/`continue` конфликтуют. Разделять на фазы.
- **`recv`-цикл без предшествующего `poll(POLLIN)`** на non-blocking сокете → мгновенный `EAGAIN`, баннер не дождались.
- **`poll(..., -1)`** (без таймаута) на молчащем сокете → висит вечно, хотя сокет non-blocking. Таймаут несёт `poll`, не режим.
- **`fcntl(F_SETFL, O_NONBLOCK)` без `F_GETFL`** → затёр прочие file status flags.

## Связанные темы

[[tcp-sockets]] [[event-loop-epoll]] [[getaddrinfo]] [[bitwise-flags]] [[stdio-buffering]]
