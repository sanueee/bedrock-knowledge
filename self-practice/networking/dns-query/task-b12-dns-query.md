---
type: atomic
block-position: B-12
tema: DNS protocol manually (A-record query поверх UDP, компрессия имён)
status: выполнено
created: 2026-07-06
code: self-practice/networking/dns-query/dnsquery.c
---

# B-12 — DNS-резолвер вручную (A-record поверх UDP)

## Задание

Написать `dnsquery` — резолвер, который **сам собирает DNS-запрос по байтам**, отправляет его на публичный резолвер (8.8.8.8:53) поверх UDP, принимает ответ и вытаскивает из него IPv4-адреса. Никакого `getaddrinfo` — вся работа с wire-форматом руками. По итогу мысленно сравнить с B-4 (`resolve.c`): что за тебя делал `getaddrinfo`.

**Запуск:** `./dnsquery example.com` → печатает список A-записей (`93.184.216.34` и т.д.).

Место: `self-practice/networking/dns-query/dnsquery.c`.

### Что делает программа (по шагам, с вызовами)

1. Готовит UDP-сокет к 8.8.8.8:53: `socket(AF_INET, SOCK_DGRAM, 0)`, адрес через `inet_pton()` + `htons(53)` в `struct sockaddr_in`.
2. Собирает **DNS-заголовок** (12 байт) в буфере: `ID` (любой), флаги с `RD=1` (recursion desired), `QDCOUNT=1`, остальные счётчики = 0. Все 16-битные поля — в сетевой порядок через `htons()`.
3. Кодирует **QNAME**: `example.com` → `7 e x a m p l e 3 c o m 0`. Каждый label — байт длины + байты label, терминатор `0`. Собираешь вручную (проход по строке, поиск `.` через `memchr()`/`strchr()`, копирование `memcpy()`).
4. Дописывает `QTYPE=A` (1) и `QCLASS=IN` (1), тоже через `htons()`.
5. Отправляет весь запрос одной датаграммой: `sendto()`.
6. Принимает ответ: `recvfrom()` → возвращает `n` байт. **Работать ровно с `n`**, буфер за `n` — мусор.
7. Разбирает заголовок ответа: читает `ANCOUNT` (`ntohs()`), проверяет флаги (бит QR=1, RCODE=0).
8. **Пропускает секцию Question**: пройти QNAME (labels до нулевого байта) + 4 байта (QTYPE+QCLASS).
9. Разбирает `ANCOUNT` **Answer-записей**. Формат RR: `NAME`, `TYPE`(2), `CLASS`(2), `TTL`(4), `RDLENGTH`(2), `RDATA`. Для `TYPE=A` `RDATA` = 4 байта IPv4 → печать через `inet_ntop()` или `%u.%u.%u.%u`.
10. **Компрессия имён**: `NAME` в ответе — почти всегда указатель. Байт со старшими битами `11` (`0xC0`) означает: следующие 14 бит — offset от начала пакета, где продолжается имя. Для пропуска RR тебе достаточно распознать указатель (2 байта) vs. последовательность labels — полностью раскручивать имя для A-записи не обязательно, но распознать и корректно перескочить — обязательно.

### Хедеры

```c
#include <stdio.h>       // printf, fprintf, perror
#include <stdlib.h>      // exit, EXIT_FAILURE
#include <string.h>      // memcpy, memchr, strlen
#include <stdint.h>      // uint8_t, uint16_t, uint32_t
#include <unistd.h>      // close
#include <arpa/inet.h>   // htons, ntohs, inet_pton, inet_ntop
#include <sys/socket.h>  // socket, sendto, recvfrom
#include <netinet/in.h>  // struct sockaddr_in, IPPROTO_UDP
```

### На чём тренируемся

- **Ручная сборка бинарного протокола**: byte layout, network byte order (`htons`/`ntohs`), где заканчивается заголовок и начинаются поля переменной длины.
- **Парсинг недоверенных сетевых данных** — горячая зона (см. [[feedback-tedious-security-code]]). Ответ пришёл из сети, ему нельзя верить:
  - каждый доступ к байту сверять с `end = buf + n`; вышел за границу — abort, не читать;
  - `RDLENGTH` из пакета — не доверять: `p + rdlength <= end` перед чтением RDATA;
  - **compression pointer может зацикливаться** (указывать назад/на себя) → бесконечный цикл при раскрутке имени = DoS. Если будешь раскручивать имя — счётчик прыжков / лимит.
- Сравнение с `getaddrinfo` (B-4): что резолвер-библиотека прятала (сборку пакета, retry, /etc/resolv.conf, компрессию).

### Security-заметки (заполнить в vault по итогам)

- Границы буфера ответа vs `n` из `recvfrom`.
- `RDLENGTH` — untrusted длина.
- Compression pointer loop → DoS.
- Type punning / alignment: если кастуешь `buf` на `struct` заголовка — padding и выравнивание; безопаснее ручное извлечение байт или `memcpy` в поле.

---

## Моё решение

Разбил на функции ради читаемого `main` (объём файла напрягал): `dns_socket` (socket+timeout+connect) → `build_query` (header+encode+QTYPE/QCLASS) → `send_query` → `recv` → три гейта (`ans_id==id`, `parse_flags`, `ancount`) → `skip_name` → `parse_answers`. Ключевое проектное решение — **одна универсальная `skip_name`**, распознающая и голые labels (имя вопроса), и compression pointer (`0xC0`) в Answer-записях; за указателем не ходит (SKIP-режим), поэтому петля указателей не страшна. Работает end-to-end: `example.com`, `dns.google` (→ 8.8.8.8/8.8.4.4), NXDOMAIN. Компилируется чисто `-Wall -Wextra`.

## Ключевые функции

| Функция | Зачем |
|---------|-------|
| `socket`/`connect`/`send`/`recv` | UDP к 8.8.8.8:53; `connect` фильтрует off-path ответы |
| `setsockopt(SO_RCVTIMEO)` | таймаут приёма — иначе `recv` виснет вечно при потере датаграммы |
| `inet_pton` / `inet_ntop` | IP резолвера в `sin_addr` / печать A-записи |
| `htons` / сдвиги `>>8`,`&0xFF` | 16-битные поля в network byte order |
| `memchr` / `memcpy` | нарезка hostname на labels |

## Что узнал

- **DNS wire format**: 12-байтовый заголовок (ID, Flags-битполе, QDCOUNT/ANCOUNT…), QNAME как length-prefixed labels (точек нет), RR = NAME/TYPE/CLASS/TTL/RDLENGTH/RDATA.
- **Битовые поля Flags**: извлечение маской+сдвигом (QR = `(flags>>15)&1`, RCODE = `flags&0x0F`); RD в запросе = `0x0100` (бит 8 — младший бит старшего байта).
- **Endianness**: сдвиги `>>8`/`&0xFF` работают над **значением**, не над байтами в памяти → переносимы; `htons`/`ntohs` внутри = byte swap (или no-op на BE), решается на компиляции.
- **Три гейта ответа**: `qr==1` → `rcode` → `ancount`. Порядок важен: `ancount==0` до `rcode` спрятал бы NXDOMAIN. NODATA (`rcode=0 && ancount=0`) ≠ NXDOMAIN (`rcode=3`).
- **Компрессия имён**: `0xC0`-указатель терминирует имя; для SKIP достаточно распознать (`(b&0xC0)==0xC0`) и перескочить 2 байта.

## Ошибки и трудности

Ответ на вопрос дебрифа: сложным были **битовые операции** (установка/извлечение флага), **endianness**, **объём программы**, и **квалификаторы указателей** — VS Code подсвечивает `char*`→`const char*` / знаковость как ошибку, mental model про это ещё сырая (на деле добавление `const` легально, ругается на снятие `const` и на разные типы `char*`/`uint8_t*`). Уроки `/check`: `skip_name` разыменовывала `*p` до проверки `p<end` (OOB); не умела компрессию (ломалась на `0xC0`); не возвращала значение (UB); порядок гейтов `ancount` до `rcode` прятал NXDOMAIN. Главный C-инсайт: `p + rdlength > end` при untrusted `rdlength` — **UB на образовании** заграничного указателя (не только на разыменовании), правильно `rdlength > end - p`.

## Что бы сделал иначе

Рандомизировать `ID` (сейчас хардкод 1234 — предсказуемый ID → spoofing). Опционально: раскрутка имён по offset (тогда пригодится параметр `msg` в `parse_answers`, сейчас unused).

## Код

`self-practice/networking/dns-query/dnsquery.c`

## Связанные темы

[[dns]] [[udp-sockets]] [[getaddrinfo]] [[feedback-tedious-security-code]]

## Ключевые термины (English)

- **wire format** — байтовое представление протокола «на проводе».
- **network byte order** — big-endian, канон для полей в пакете.
- **name compression** — переиспользование имени через `0xC0`-указатель.
- **NXDOMAIN / NODATA** — имени нет vs имя есть без записи нужного типа.
- **out-of-bounds pointer formation** — образование указателя за пределы массива = UB, до разыменования.
