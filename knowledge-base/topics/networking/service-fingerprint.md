---
тема: Service fingerprinting — идентификация сервиса по пробе
блок: B (сетевой стек)
task: [[task-b13-service-fingerprint]]
код: sections/networking/service-fingerprint/fingerprint.c
дата: 2026-07-12
---

# Service fingerprinting

Определить, **что реально слушает** на `host:port`, послав пробу и распознав ответ.
Не lookup порта в `/etc/services` — сервис может висеть где угодно (SSH на 2222, HTTP на 8080).

## Организующий принцип: решает ответ, не порт

- **Порт → выбирает пробу** (эвристика «на 80 логично начать с HTTP»).
- **Ответ → классифицирует** (паттерн `SSH-`/`HTTP/`/`220 ` в теле).

Свойство протокола — **кто говорит первым** после `connect`:

| Класс | Протоколы | Действие | `send_first` |
|-------|-----------|----------|--------------|
| server-first | SSH (RFC 4253), SMTP, FTP | сразу `recv` баннер | 0 |
| client-first | HTTP | сначала `send` запрос, потом `recv` | 1 |

HTTP-сервер молчит, пока не пришлёшь запрос → если ждать баннер как от server-first,
`recv` упрётся в таймаут → false negative. Это кодируется флагом `send_first` в `struct probe`.

## Таблица проб (const-данные)

```
порт(ы)        класс          payload                  pattern   service
22             server-first   NULL                     "SSH-"    ssh
80/8080/8443   client-first   "HEAD / HTTP/1.0\r\n\r\n" "HTTP/"   http
25/587         server-first   NULL                     "220 "    smtp
21             server-first   NULL                     "220 "    ftp
```

- `HTTP/1.0` в payload намеренно: `1.1` требует заголовок `Host:`, `1.0` — нет.
- SMTP и FTP оба несут `"220 "` → по содержимому неразличимы; здесь их разводит **порт**
  (через выбор пробы), паттерн лишь подтверждает семейство. Строгое различение — по `SMTP`/`ESMTP` в greeting.

## Пайплайн (fingerprint.c)

1. `probes_for_port(port, &n)` → указатель на пробу(ы) для порта (или `n==0` → unknown).
2. `connect_timeout(host, port, ms)` → живой fd:
   - `O_NONBLOCK` + `connect` → `EINPROGRESS`;
   - `select` на writable с таймаутом; `n==0` → filtered;
   - **`getsockopt(SO_ERROR)`** — writable ≠ connected (`ECONNREFUSED` тоже делает сокет готовым).
3. `run_probe(fd, pr, buf, buflen)`:
   - **снять `O_NONBLOCK`** (иначе `SO_RCVTIMEO` не действует, `recv` вернёт `EAGAIN` мгновенно);
   - `SO_RCVTIMEO`;
   - при `send_first` — `send` payload в цикле-аккумуляторе (partial send);
   - `recv` баннер, вернуть `n`.
4. `memmem(buf, n, pr->pattern, strlen(pr->pattern))` в `main` → сервис; печать `%.*s` по `n`.

## Security / грабли

- **`strlen(NULL)` → SIGSEGV** на server-first пробах (`payload==NULL`). `strlen` держать
  внутри `if (send_first)`. Не ловится компилятором и чтением — только прогоном.
- **`recv` не кладёт `\0`**: `memmem` по длине `n`, печать `%.*s`, никаких `strstr`/`%s`.
- **partial send**: `send` может отправить меньше → цикл `while (total < len)` с аккумулятором
  (`total` и `sent` — **разные** переменные, оба инициализировать нулём).
- **усечение порта**: валидировать диапазон на `long` до сужения в `uint16_t`.
- **fd leak**: `close(fd)` на каждой ветке, которая не возвращает fd.
- **`timeout_ms`** — это миллисекунды: `3` = 3 мс (флейк на интернет-RTT), нужно `3000`.

## Ключевые термины (English)

- **service fingerprinting**, **banner grabbing**, **server-first / client-first**,
  **probe**, **`SO_RCVTIMEO`**, **partial send**, **writable ≠ connected**.

## См. также

- [[connect-timeout-scan]] — неблокирующий connect + select (B-8), база `connect_timeout`.
- [[http]] — HTTP framing (B-11), откуда HTTP-проба.
- [[tcp-sockets]] — `send`/`recv` семантика.
- [[feedback-tedious-security-code]] — механика error-handling = зона механических багов.
