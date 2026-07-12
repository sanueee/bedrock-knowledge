---
type: atomic
block-position: B-9
tema: UDP sockets — recvfrom/sendto, connectionless, датаграммные границы, потеря пакетов
status: выполнено
created: 2026-07-03
code: sections/networking/udp-echo/udp_echo.c
---

# task-b09 — UDP echo (recvfrom / sendto)

## Задание

Написать `sections/networking/udp-echo/udp_echo.c` — **одна программа, два режима** (server / client),
выбор по argv. Цель — прочувствовать чем UDP отличается от TCP из B-2 на уровне
системных вызовов, а не концептуально.

### Режим server: `./udp_echo server <port>`

1. `socket(AF_INET, SOCK_DGRAM, 0)` — обрати внимание: `SOCK_DGRAM`, не `SOCK_STREAM`.
2. `bind()` на `INADDR_ANY:<port>`.
3. **Никаких `listen()` / `accept()`** — их для UDP не существует. Сразу в цикл.
4. Цикл: `recvfrom()` → получить датаграмму **и адрес отправителя** в `struct sockaddr_in`
   → распечатать `ip:port` отправителя (через `inet_ntop` + `ntohs`) и тело →
   `sendto()` то же тело обратно **на тот адрес, что заполнил recvfrom**.
5. Буфер приёма сделай **специально маленьким — 8 байт**. Это нужно для пункта
   «датаграммные границы» ниже: увидишь усечение (truncation), а не «дочитывание».

### Режим client: `./udp_echo client <ip> <port> <message>`

1. `socket(AF_INET, SOCK_DGRAM, 0)`.
2. Собрать `struct sockaddr_in` получателя вручную: `inet_pton()` для IP, `htons()` для порта.
   **Без `connect()`** — адрес назначения передаём прямо в `sendto()`.
3. `sendto()` — отправить `<message>`.
4. Поставить на приёмный сокет **тайм-аут** через
   `setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv)` (`struct timeval`, напр. 2 сек).
5. `recvfrom()` ответа:
   - пришёл → распечатать эхо;
   - вернул `-1` и `errno == EAGAIN` (или `EWOULDBLOCK`) → напечатать
     **`packet lost (timeout)`** и выйти с ненулевым кодом. Это и есть
     «потеря пакета как штатное событие» — не падение, а обычная ветка логики.

### Что задание обязано показать (проверь на прогоне)

1. **Connectionless.** Server не зовёт `accept()`, один сокет обслуживает любого клиента.
   Запусти server, потом дважды подряд client с разных «сообщений» — оба обслужены
   тем же сокетом без переустановки соединения.
2. **Датаграммные границы (нет partial read).** Client отправляет строку длиннее 8 байт
   (скажем `"hello-world"`, 11 байт). Server с 8-байтным буфером получит её **за один
   `recvfrom`, усечённой до 8 байт** — остаток датаграммы **отброшен**, а не остался
   «дочитаться» следующим вызовом (в TCP-echo из B-2 было бы наоборот). Подтверди
   флагом `MSG_TRUNC` в `recvmsg`/через возвращаемую длину, либо просто покажи что
   пришло `hello-wo` и хвост потерян.
3. **Потеря пакета.** Запусти client, когда server **не запущен** (или на closed-порт) —
   `sendto` пройдёт без ошибки (UDP не подтверждает доставку), а `recvfrom` упрётся
   в тайм-аут → `packet lost`. Сравни: в TCP `connect` на closed-порт дал бы
   `ECONNREFUSED` сразу (см. B-8 `connect_timeout`).

### Хедеры (подключить явно)

- `<sys/socket.h>` — `socket`, `bind`, `recvfrom`, `sendto`, `setsockopt`, `SOCK_DGRAM`, `SOL_SOCKET`
- `<netinet/in.h>` — `struct sockaddr_in`, `INADDR_ANY`, `htons`
- `<arpa/inet.h>` — `inet_pton`, `inet_ntop`, `ntohs`
- `<sys/time.h>` — `struct timeval` (для `SO_RCVTIMEO`)
- `<errno.h>` — `errno`, `EAGAIN` / `EWOULDBLOCK`
- `<string.h>`, `<stdio.h>`, `<stdlib.h>`, `<unistd.h>`

### Скелет (тела пишешь сам)

```c
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFSZ 8   /* специально маленький — для демонстрации усечения */

static int run_server(const char *port_str);          /* bind + loop recvfrom/sendto */
static int run_client(const char *ip, const char *port_str, const char *msg);

int main(int argc, char **argv) {
    /* argv[1] == "server" | "client", диспетчеризация на run_* */
}
```

### Прогон (Docker gcc, как в B-8)

- Терминал 1: `./udp_echo server 9999`
- Терминал 2: `./udp_echo client 127.0.0.1 9999 hello-world` → ждём усечения на сервере + эхо клиенту
- Терминал 2 (server выключен): `./udp_echo client 127.0.0.1 9999 test` → `packet lost (timeout)`

---

## Моё решение

Одна программа `udp_echo.c`, два режима по `argv[1]` (`server`/`client`).

- **Server:** `socket(SOCK_DGRAM)` → `bind(INADDR_ANY)` → **без** `listen`/`accept` →
  `while(!stop)` цикл `recvfrom` (буфер `BUFSZ 8`, `src`/`srclen` внутри цикла) → печать
  отправителя (`inet_ntop` + `ntohs`) → `sendto(buf, n, ...)` обратно на `src`. Грациозное
  завершение — `sigaction(SIGINT)` со `sa_flags = 0` (без `SA_RESTART`): `recvfrom` → `EINTR`
  → `continue` → `!stop` выпускает из цикла → `close`.
- **Client:** `socket(SOCK_DGRAM)` → собрать `sockaddr_in` вручную (`memset` + `sin_family` +
  `inet_pton` + `htons`) → `sendto(msg, strlen(msg))` **без** `connect` → `setsockopt(SO_RCVTIMEO, 2s)`
  → `recvfrom` ответа: `n>0` печать `%.*s`, `EAGAIN`/`EWOULDBLOCK` → `packet lost` + `return 1`.
- `main` пробрасывает `res` (`return res`) — код выхода клиента доходит до shell.

Все три теста прошли: connectionless (два клиента, разные эфемерные порты), усечение
(`hello-world` → `hello-wo`, хвост потерян), потеря пакета (сервер выключен → timeout, exit 1).

## Ключевые функции

| Функция | Зачем |
|---------|-------|
| `socket(AF_INET, SOCK_DGRAM, 0)` | UDP-сокет (датаграммный, не потоковый) |
| `recvfrom(fd, buf, len, 0, &src, &srclen)` | приём **одной** датаграммы + адрес отправителя; `srclen` value-result |
| `sendto(fd, buf, n, 0, &dst, dstlen)` | отправка датаграммы на явный адрес, без соединения |
| `setsockopt(SO_RCVTIMEO)` | тайм-аут на блокирующий `recvfrom` → потеря пакета как `EAGAIN` |
| `inet_pton` / `inet_ntop` | текст↔бинарь IP; `inet_ntop` **гарантирует** терминатор, `recvfrom` — нет |
| `sigaction(SIGINT)` без `SA_RESTART` | прерывание `recvfrom` для grateful shutdown |

## Что узнал

- UDP-сервер не имеет соединения: один сокет, `recvfrom` кладёт адрес отправителя в `src`,
  ответ шлёшь туда же. Нет `listen`/`accept`/состояния per-client.
- **Датаграммные границы:** один `recvfrom` = одна датаграмма. Буфер меньше датаграммы →
  **усечение** (остаток отброшен), а не partial read как в TCP-потоке.
- **Потеря пакета — штатное событие:** `sendto` на мёртвый порт проходит молча (нет ACK),
  ловишь отсутствие ответа тайм-аутом. Контраст с TCP `connect` → мгновенный `ECONNREFUSED`.
- `%s` безопасен только над гарантированной C-строкой: `inet_ntop` терминатор ставит,
  `recvfrom` — никогда. Печать сетевых данных — `%.*s` с длиной `n`, не `%s`.

## Ошибки и трудности

**Главный затык (со слов пользователя): механика подсчёта байт** — какие возвраты что
означают и как не выйти за границы. Конкретно всплыло:
- `printf("%s", buf)` по буферу из `recvfrom` — OOB-read, если датаграмма заполнила буфер
  без `'\0'`. Исправлено на `%.*s` с `(int)n`.
- Доверие «ожидаемой» длине вместо возвращённого `n`: трактовать надо ровно `n` байт.
- `sizeof(msg)` дал бы 8 (указатель), длина строки — только `strlen`.

Второй затык: **инициализация `sockaddr_in`** — в клиенте забыл `memset` + `sin_family = AF_INET`
(в сервере сделал). Без `sin_family` ядро не знает семейство адреса → `sendto` не отправит.

Мелочи: `return res` в `main` пришлось напомнить дважды (иначе exit-код клиента терялся).

## Что бы сделал иначе

Инициализировать `sockaddr_in` сразу шаблоном (`memset` + `sin_family`) для обоих режимов,
не по месту. При печати любых данных из сети — сразу `%.*s`, не `%s`, как рефлекс.

## Код

`sections/networking/udp-echo/udp_echo.c`

## Связанные темы

[[udp-sockets]] [[tcp-sockets]] [[connect-timeout-scan]] [[getaddrinfo]]
