---
type: atomic
block-position: B-8
phase: Фаза 2 — Non-blocking I/O и connect-scan
status: done
created: 2026-07-02
completed: 2026-07-02
---

# B-8 — Connect-with-timeout pattern

Атомарное задание блока B (сетевой стек, C). Фаза 2.
**Синтезирует пройденное:** B-6 (non-blocking connect / `EINPROGRESS`) + B-7 (epoll / `EPOLLOUT`).
Это **боевой примитив connect-scan'а**: одна проба одного порта с таймаутом, без блокировки и без root. Прямо ляжет в B-10 (port-scan v1) и в магнум опус.

Тема: как проверить доступность TCP-порта, не подвиснув на дефолтном таймауте ядра (~разные десятки секунд) и не блокируя поток — non-blocking `connect` → `epoll` ждёт `EPOLLOUT` с нашим таймаутом → `getsockopt(SO_ERROR)` говорит, чем на самом деле кончилось.

---

## Что написать

Утилита `connect_timeout`: пробить один `host:port` с заданным таймаутом (мс), напечатать результат — `open` / `closed` / `filtered (timeout)`.

Код: `self-practice/networking/connect-timeout/connect_timeout.c` (можно расти из `nbconnect.c` из B-6, но лучше отдельный файл — чище для ревью).
epoll — Linux-only, гоняем в Docker gcc-контейнере, как B-7.

Вызов: `./connect_timeout <ip> <port> <timeout_ms>`, например `./connect_timeout 127.0.0.1 22 300`.

### Три исхода — и как их различить
- **open** — `connect` завершился успешно: после `EPOLLOUT` `SO_ERROR == 0`.
- **closed** — хост ответил RST: после `EPOLLOUT` `SO_ERROR == ECONNREFUSED`.
- **filtered/timeout** — `epoll_wait` вернул 0 за отведённое время (ответа нет вообще — фаервол дропает).

**Ключевой нюанс (не проскочи):** сокет становится «writable» (`EPOLLOUT`) и при успехе, и при отказе. Сам факт `EPOLLOUT` **не** значит «порт открыт». Различает только `getsockopt(SO_ERROR)`. Если поверить `EPOLLOUT` — закрытые порты покажутся открытыми. Это сердце задания.

---

## Механика по шагам (словами, без кода)

1. `socket(AF_INET, SOCK_STREAM, 0)`.
2. Перевести fd в non-blocking: `fcntl(fd, F_GETFL)` → `fcntl(fd, F_SETFL, flags | O_NONBLOCK)`.
3. Заполнить `struct sockaddr_in` (адрес через `inet_pton`, порт через `htons`).
4. `connect()`. Ожидаемо вернёт `-1` **и** `errno == EINPROGRESS` — это норма для non-blocking (соединение «поехало» в фоне). Если вернул `0` сразу — уже соединился (бывает на loopback). Любой другой `errno` — реальная ошибка.
5. `epoll_create1(0)`, зарегистрировать fd на `EPOLLOUT` (`epoll_ctl` + `EPOLL_CTL_ADD`).
6. `epoll_wait(epfd, &ev, 1, timeout_ms)`:
   - `== 0` → таймаут → **filtered**.
   - `> 0` → сокет готов → шаг 7.
   - `== -1` и `errno == EINTR` → повторить `epoll_wait` (сигнал прервал — не считать за ошибку).
7. `getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len)` — забрать отложенную ошибку соединения:
   - `err == 0` → **open**.
   - `err == ECONNREFUSED` → **closed**.
   - иначе → печатай `strerror(err)` (например `EHOSTUNREACH`).
8. Закрыть **всё** на **каждом** пути выхода (epoll fd и socket fd).

---

## Хедеры (подключить явно)

- `<sys/socket.h>` — `socket`, `connect`, `getsockopt`, `SOL_SOCKET`, `SO_ERROR`
- `<sys/epoll.h>` — `epoll_create1`, `epoll_ctl`, `epoll_wait`, `EPOLLOUT`
- `<fcntl.h>` — `fcntl`, `F_GETFL`/`F_SETFL`, `O_NONBLOCK`
- `<errno.h>` — `errno`, `EINPROGRESS`, `ECONNREFUSED`, `EINTR`
- `<netinet/in.h>` / `<arpa/inet.h>` — `struct sockaddr_in`, `htons`, `inet_pton`
- `<unistd.h>` — `close`
- `<string.h>` — `memset`, `strerror` | `<stdio.h>`, `<stdlib.h>` — I/O, `atoi`/`strtol`

---

## Скелет (сигнатуры — тела пишешь сам)

```c
// исходы пробы
typedef enum {
    PROBE_OPEN,
    PROBE_CLOSED,
    PROBE_FILTERED,
    PROBE_ERROR
} probe_result_t;

// одна проба host:port с таймаутом. Возвращает исход.
// внутри: non-blocking connect -> epoll EPOLLOUT -> getsockopt(SO_ERROR)
probe_result_t probe_port(const char *ip, int port, int timeout_ms);

int main(int argc, char **argv) {
    // разобрать argv: ip, port, timeout_ms
    // вызвать probe_port, напечатать open/closed/filtered
    return 0;
}
```

Скелет минимальный намеренно — тело `probe_port`, разбор аргументов, обработку всех веток `errno` и печать пишешь сам.

---

## Security-фокус (§4)

- **Утечка fd = потолок сканера.** Connect-scan открывает сокет на **каждый** порт. Забыл `close` на ветке ошибки/таймаута — упрёшься в `RLIMIT_NOFILE` уже на первой сотне портов, дальнейшие пробы фейкнут `EMFILE`. `close` обязателен на **всех** путях выхода (early return тоже). Это тот же класс fd-leak, что ловили в ревью B-7.
- **`EPOLLOUT` ≠ успех — доверять только `SO_ERROR`.** Логическая уязвимость: если сканер рапортует порт «open» по факту readiness, а не по `SO_ERROR == 0`, он врёт про поверхность атаки (closed → «open»). Корректность здесь = корректность разведданных.
- **`EINTR` на `epoll_wait`** — если не перезапустить, сигнал (даже безобидный) превратится в ложный `filtered`. Штатное событие, не ошибка.
- **Валидация argv:** `port` вне `1..65535`, мусорный `timeout_ms` — отсеки до `probe_port` (недоверенный ввод). `atoi` молча вернёт 0 на мусоре — лучше `strtol` с проверкой.

---

## Где сохранить

- Код: `self-practice/networking/connect-timeout/connect_timeout.c`
- Конспект после ревью: `topics/networking/` (через `vault-write`) + блок `## Ключевые термины (English)`: non-blocking connect, `EINPROGRESS`, connect-with-timeout, `EPOLLOUT` readiness, `SO_ERROR` (pending socket error), `getsockopt`, filtered vs closed, fd exhaustion.

## Ревью / Разбор

**Итог:** решено, `probe_port` корректен на всех трёх исходах. Проверено end-to-end в Docker (gcc-контейнер, epoll Linux-only): `open` (127.0.0.1:75 со слушателем `epoll_echo &`), `closed` (127.0.0.1:9/75/8080 без слушателя). Конспект: [[connect-timeout-scan]].

**Что сделано верно:** развилка `connect()` на три исхода (`0` мгновенный успех / `EINPROGRESS` → epoll / прочий `errno` → ERROR); вердикт снимается с `getsockopt(SO_ERROR)`, а не с факта `EPOLLOUT`; ветки ошибок закрывают `fd`+`epfd` и возвращаются; `getsockopt` `0`→open / `ECONNREFUSED`→closed.

**Ошибки (3 итерации /check, все механические, не концептуальные):**
1. **`while(...EINTR)` без тела — 2 итерации.** `while(cond) if(...)...` без `{}`/`;`: телом цикла стал весь `if/else`, выполнялся только во время EINTR-спинов, а на нормальном исходе (`n==0`/`n>0`) условие ложно → цикл выходит **не выполнив тело** → `PROBE_FILTERED` недостижим, `epfd` течёт. Фикс: пустое тело `;`, разбор `n` **после** цикла. Осознанность подтверждена (объяснил «почему» словами).
2. Приоритет операторов: `int res = connect(...) == -1 && errno == EINPROGRESS;` — `res` получал результат сравнения (0/1), а не возврат `connect()`. Разделено на `int ret = connect(...)` + отдельные проверки.
3. Валидация порта не с первого раза (`>65535` без нижней границы `0`); `main` сначала не печатал результат.

**Окружение-урок:** под Docker Desktop (macOS) `filtered` не тестируется — userspace NAT-прокси (vpnkit/gvproxy) ACK'ает всё, любой адрес выглядит `open`. Детали → [[connect-timeout-scan]].

**Рефлексия (со слов):** лёгкое задание, концептуальных затыков нет — механизм connect-timeout освежён (non-blocking connect → `EPOLLOUT` → `SO_ERROR`). Пробуксовки только синтаксические (`while` без тела, приоритет операторов).
