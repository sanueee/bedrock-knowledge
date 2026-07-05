---
type: atomic
block-position: B-11
блок: B — Сетевой стек (C)
фаза: 3 — Application protocols + service detection
тема: HTTP basics — минимальный клиент поверх TCP
статус: выполнено
создано: 2026-07-05
выполнено: 2026-07-05
код: networking/httpget.c
---

# B-11 (атомарное) — HTTP basics: минимальный HTTP/1.1-клиент

**Тип:** атомарное, позиция 11/26, Фаза 3 (application protocols).
**Одна тема:** семантика HTTP/1.1 поверх готового TCP-транспорта. Сокеты ты уже умеешь (B-2/B-4) — здесь тренируется **разбор протокола прикладного уровня**, а не механика `connect()`.

Не путать с B-5 (`host_probe`, сырой banner grab — читаем что сервер прислал первым). Здесь ты **сам конструируешь корректный HTTP-запрос** и **парсишь структурированный ответ**: status line, заголовки, тело по `Content-Length` **или** chunked.

---

## Задание

Написать `networking/httpget.c` — консольный HTTP/1.1-клиент, достаточный для banner grab.

### Что принимает (argv)
```
./httpget <host> [path] [port]
```
- `host` — имя (`example.com`) или IP. Резолвить через `getaddrinfo` (переиспользуй подход из B-4).
- `path` — по умолчанию `/`.
- `port` — по умолчанию `80`.

### Что делает
1. Резолвит host → TCP-connect (можно синхронный `connect`, non-blocking здесь не требуется).
2. Конструирует и отправляет **вручную** минимальный валидный запрос:
   ```
   GET <path> HTTP/1.1\r\n
   Host: <host>\r\n
   User-Agent: httpget/0.1\r\n
   Connection: close\r\n
   \r\n
   ```
   - `Host:` — **обязателен** в HTTP/1.1 (иначе 400 на name-based virtual hosting).
   - `Connection: close` — просим сервер закрыть соединение после ответа. Это упрощает чтение тела: конец потока = конец тела (для не-chunked). Осознай почему.
   - Пустая строка (`\r\n` после последнего заголовка) — конец секции заголовков. Без неё сервер ждёт продолжения.
3. Читает ответ полностью (цикл `recv` до `n==0`, помни B-9: `recv` возвращает **ровно** прочитанное, не «ожидаемое»).
4. **Парсит:**
   - **Status line:** `HTTP/1.1 200 OK` → выделить код (int) и reason-phrase. Печатать.
   - **Заголовки:** до пустой строки `\r\n\r\n`. Найти и напечатать `Server:` (если есть). Найти `Content-Length:` и `Transfer-Encoding: chunked` (регистр имени заголовка **case-insensitive** по RFC — учти).
   - **Тело:**
     - если есть `Content-Length: N` → тело ровно `N` байт после заголовков;
     - если `Transfer-Encoding: chunked` → **раскодировать chunked**: каждый чанк = строка с размером в hex + `\r\n` + данные + `\r\n`, терминатор — чанк размера `0`.
5. Флаг `HEAD` (по желанию, для +балла): если `argv` содержит `-I`/`--head` — слать `HEAD` вместо `GET`. У `HEAD` тела нет по определению — сервер шлёт только заголовки. Полезно для banner grab (не тянуть тело).

### Вывод (пример)
```
$ ./httpget example.com
Status: 200 OK
Server: ECS (dcb/7F84)
Content-Length: 1256
--- body (1256 bytes) ---
<!doctype html> ...
```

---

## Хедеры (copy-paste в начало файла)

```c
#include <stdio.h>      // printf, fprintf, snprintf
#include <stdlib.h>     // strtol (код статуса, hex-размер чанка), exit
#include <string.h>     // strstr, strncmp, memcpy, strlen
#include <strings.h>    // strcasecmp, strncasecmp (case-insensitive имена заголовков)
#include <errno.h>      // errno
#include <unistd.h>     // close
#include <netdb.h>      // getaddrinfo, freeaddrinfo, gai_strerror, struct addrinfo
#include <sys/socket.h> // socket, connect, recv, send, struct sockaddr, AF_*, SOCK_STREAM
#include <arpa/inet.h>  // inet_ntop (если печатаешь результат резолва)
```

---

## На чём тренируешься (фокус)

- **Формат HTTP-запроса:** ручная сборка, роль `Host`, `Connection: close`, CRLF-разделители, пустая строка-терминатор заголовков. Один пропущенный `\r\n` = зависший запрос.
- **Парсинг ответа без готовой либы:** граница заголовки/тело по `\r\n\r\n`; case-insensitive имена заголовков; извлечение числовых значений.
- **Content-Length vs chunked** — два разных способа обозначить длину тела. Понять **зачем** нужен chunked (сервер не знает длину заранее — генерирует на лету).
- **Границы буфера (security §4):** ответ читается в буфер — не переполни; при парсинге `Content-Length` не доверяй значению вслепую (заявленный `N` может врать/переполнять — сравнивай с фактически прочитанным). `%s` по сетевым данным без терминатора = OOB-read; печатать тело через `%.*s` с явной длиной.

---

## Security-заметки (§4, заполнить при ревью)

- Валидация `Content-Length`: заявленная длина vs фактически полученные байты — не читать «до N» доверяя серверу.
- Буфер приёма: фиксированный размер vs динамический рост; переполнение при большом ответе.
- Chunked: hex-размер чанка через `strtol` — проверять на overflow и мусорные символы.
- Печать тела/заголовков: только `%.*s`, сетевые данные не NUL-terminated.

---

## Где сохранить

- **Код:** `networking/httpget.c`
- **Конспект (после сессии, `vault-write`):** `topics/networking/http.md` — новый, темы нет. Блок «Ключевые термины (English)» обязателен (request line, status line, header field, chunked transfer-encoding, entity body).

---

## Итоги (заполнено по сессии 2026-07-05)

### Что реализовано

`networking/httpget.c` — HTTP/1.1-клиент полного цикла (`./httpget <host> <path> <port>`):
- **`tcp_connect`** — `getaddrinfo(AF_UNSPEC, SOCK_STREAM)` + **перебор списка кандидатов** (`ai_next`): `socket`/`connect` на каждом узле, первый успех → `return fd`, весь список без успеха → `-1`. `freeaddrinfo` на обоих путях, `close` неудавшегося `fd`.
- **Сборка запроса** — `snprintf` (GET + `Host` + `Connection: close` + пустая строка), отправка `send`-циклом до `sent == n` с проверкой `-1`.
- **Приём** — `recv`-цикл до `0` (полагаемся на `Connection: close`), проверка `ans == -1`.
- **`parse_headers`** — обход строк по `\r\n` через `memmem`, guard `eol==NULL → sep` (последний заголовок), status line по дискриминатору `line == buf`, заголовки через `strncasecmp`. Возвращает `enum { FRAMING_CHUNKED, FRAMING_LENGTH, FRAMING_CLOSE }` + `content_length` через out-параметр.
- **Трёхсторонняя развилка тела** в `main`: LENGTH → печать `min(content_length, принятое)`; CLOSE → печать всего принятого; CHUNKED → `decode_chunked`. Приоритет chunked (RFC).
- **`decode_chunked`** — цикл `strtol(p, &endptr, 16)` (hex-размер) → проверка `endptr==p` (мусор) → `size==0` (терминатор) → проверка границы `size > end - p` → печать `%.*s` → `p += size + 2`.

Собирается `gcc -Wall -Wextra` (один cosmetic warning sign-compare на строке 80). Все ветки проверены end-to-end.

### Баги и разбор (ревью /check, 4 итерации)

Найдено и закрыто:
1. **`connect(fd, &p->ai_addr, ...)`** — `&` на том, что уже указатель (`ai_addr` — `struct sockaddr *`) → передавался `sockaddr **`. Убрать `&` и каст.
2. **`buf[++total_ans]='\0'`** — pre-increment → пропуск ячейки + OOB-write при полном буфере. Позже терминатор убран вовсе — перешли на length-based (`memmem`/`%.*s`), `\0` не нужен на бинарных сетевых данных.
3. **Status line печаталась на каждой строке цикла** — разбор status сидел в теле `while` без условия. Дискриминатор `line == buf`.
4. **`memmem` на последнем заголовке → NULL** — его `\r\n` попадает в первые 2 байта `sep` (вне диапазона поиска). Guard `eol==NULL → eol=sep`.
5. **`"Content-Lenght"`** — опечатка + неверная длина сравнения → заголовок не детектился.
6. **`*content_length` не записывался** (регрессия после рефактора) → `main` получал мусор → OOB-read/утечка при печати тела.
7. **`int size = strtol(...,16)`** — большой hex (`ffffffff`) переполняет `int` в отрицательное → проверка границы проходит → OOB. Фикс: `long size` + сравнение `size > end - p` (а не `p + size > end` — переполнение указателя = UB).
8. **`memchr(' ')` без NULL-проверки** — status line без пробела → SIGSEGV (подтверждён боевым тестом, exit 139). Рецидив, закрыт на 4-й итерации.

### Ключевые выводы

- **HTTP живёт не в сисколлах, а в парсинге строк** — `send`/`recv` те же, что в echo. Framing (граница сообщения в байтовом потоке) — центральная проблема, решается тремя способами.
- **Length-based парсинг > NUL-based** на сетевых данных: `memmem`/`%.*s`/`strncasecmp` по явной длине, без доверия к `\0` (тело бинарно).
- **Самый скучный код — самый security-критичный.** Границы, overflow, неинициализированные значения — именно там баги с максимальной ценой (OOB-read = утечка памяти процесса). Скука ≠ «доверь LLM», а «замедлись». См. [[feedback-tedious-security-code]].
- **Тест-класс для парсеров:** по кейсу на каждую ветку протокола + malformed + «злой» вход, ломающий границы (обрезанный chunk, огромный `Content-Length`, битая status line).
