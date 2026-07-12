---
тема: UDP-сокеты / UDP sockets — recvfrom/sendto, connectionless, датаграммные границы
блок: B — Сетевой стек
дата: 2026-07-03
связано:
  - "[[tcp-sockets]]"
  - "[[connect-timeout-scan]]"
  - "[[getaddrinfo]]"
  - "[[nonblocking-poll]]"
---

# UDP-сокеты / UDP sockets

## Что это

Датаграммный (datagram) транспорт поверх IP: `SOCK_DGRAM` вместо `SOCK_STREAM`.
Без соединения (connectionless), без гарантии доставки и порядка, без потока байт —
единица обмена это **датаграмма** (whole message), а не поток.

## Ключевые термины (English)

- **datagram** — самодостаточное сообщение; либо доставлено целиком, либо не доставлено.
- **connectionless** — нет `connect`/`listen`/`accept`; адрес назначения указывается в каждом `sendto`.
- **`recvfrom` / `sendto`** — приём/отправка датаграммы с явным адресом peer'а.
- **datagram boundary** — границы сообщения сохраняются: один `recvfrom` = одна датаграмма.
- **truncation** — если буфер меньше датаграммы, остаток **отбрасывается** (не «дочитывается»).
- **ephemeral port** — временный порт источника, который ядро назначает клиенту автоматически.
- **`SO_RCVTIMEO`** — тайм-аут на блокирующий приём; истёк → `recvfrom` вернёт `-1`, `errno == EAGAIN`.
- **value-result argument** — `socklen_t *srclen`: на входе размер буфера, на выходе фактическая длина.

## Как работает

**Сервер (без соединения):**
```
socket(AF_INET, SOCK_DGRAM, 0) → bind(INADDR_ANY:port) → loop { recvfrom → sendto }
```
Нет `listen`/`accept` — их для UDP не существует. У сокета в ядре очередь приёма
**целых датаграмм**. `recvfrom` блокируется до появления датаграммы, забирает ровно одну,
копирует тело в буфер и **отдельно** заполняет `struct sockaddr_in src` адресом отправителя.
Ответ шлёшь `sendto(..., &src, srclen)` — туда, откуда пришло. Состояние между итерациями
не хранится: каждая датаграмма самодостаточна, один сокет обслуживает всех.

**Клиент (без `connect`):**
```
socket → собрать sockaddr_in вручную (sin_family, inet_pton, htons) → sendto(&dst) → recvfrom
```
Адрес назначения передаётся прямо в `sendto`. Порт источника ядро назначает автоматически
(ephemeral) — поэтому при каждом запуске клиента сервер видит **разный** порт отправителя.

**Три свойства, отличающие от TCP:**
1. **Connectionless** — один сокет, любой клиент, без рукопожатия.
2. **Датаграммные границы** — `recvfrom` отдаёт ровно одну датаграмму. Буфер мал →
   усечение, а не partial read.
3. **Потеря пакета — штатное событие** — `sendto` на мёртвый порт проходит молча
   (нет подтверждения доставки); отсутствие ответа ловишь тайм-аутом `SO_RCVTIMEO` → `EAGAIN`.

## Пример

Из `sections/networking/udp-echo/udp_echo.c` (задание [[task-b09-udp-echo]]).

Сервер — цикл приёма с grateful shutdown:
```c
while (!stop) {
    struct sockaddr_in src;
    socklen_t srclen = sizeof(src);          /* value-result: переинициализировать КАЖДУЮ итерацию */
    char buf[BUFSZ] = {0};                    /* BUFSZ = 8, специально мал для демонстрации усечения */
    ssize_t n = recvfrom(server, buf, BUFSZ, 0, (struct sockaddr *)&src, &srclen);
    if (n < 0) { if (errno == EINTR) continue; perror("recvfrom"); break; }
    sendto(server, buf, n, 0, (struct sockaddr *)&src, srclen);   /* шлём n, не BUFSZ */
}
```

Клиент — отправка + приём с тайм-аутом:
```c
struct timeval tv = { .tv_sec = 2, .tv_usec = 0 };
setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
ssize_t n = recvfrom(client, buf, BUFSZ, 0, (struct sockaddr *)&src, &srclen);
if (n > 0) printf("%.*s\n", (int)n, buf);                        /* %.*s, НЕ %s */
else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) { /* packet lost */ }
```

Прогон: `hello-world` (11 б) на сервере с `BUFSZ 8` → пришло `hello-wo`, хвост `rld`
потерян за один `recvfrom` (усечение). Сервер выключен → клиент 2с ждёт → `timeout`, exit 1.

## Подводные камни

- **Печать `recvfrom`-буфера через `%s` — OOB-read.** `recvfrom` не ставит `'\0'`. Если
  датаграмма заполнила буфер целиком, `%s` читает за границей. Только `%.*s` с длиной `n`
  (или руками `buf[n] = '\0'`, но тогда буфер на 1 байт больше читаемого). `inet_ntop`
  терминатор гарантирует — с ним `%s` безопасен; `recvfrom`/`recv`/`read` — никогда.
- **Трактовать ровно `n` байт**, а не «ожидаемую» длину. `n` — сколько реально скопировано
  (может быть меньше датаграммы при усечении, может быть 0 при пустой датаграмме).
- **`sizeof(msg)` вместо `strlen(msg)`** для отправки строки: `msg` это указатель, `sizeof` = 8.
- **`sin_family` не выставлен** в адресе назначения → `sendto` не отправит (ядро не знает
  семейство). Всегда `memset` + `sin_family = AF_INET` перед `inet_pton`/`htons`.
- **`srclen` не переинициализирован** каждую итерацию цикла → ядро может ужать буфер адреса.
- **`inet_pton` возврат:** `1` успех, `0` невалидный формат (errno **не** установлен, `perror` соврёт),
  `-1` ошибка семейства. Ветки `0` и `-1` разделять.

## Связанные темы

[[tcp-sockets]] [[connect-timeout-scan]] [[getaddrinfo]] [[nonblocking-poll]] [[task-b09-udp-echo]]
