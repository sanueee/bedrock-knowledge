---
тема: POSIX / C — словарь сокращений в именах типов и функций
блок: C / B — сквозной справочник
дата: 2026-06-05
связано:
  - "[[getaddrinfo]]"
  - "[[tcp-sockets]]"
  - "[[error-handling-c]]"
---

# POSIX / C — словарь сокращений в именах

## Зачем эта запись

Рефлексия из B-4: сама логика не сложная, тяжело — **обилие структур и типов**, не всегда сразу вспоминается, какую брать (`sockaddr` / `sockaddr_in` / `sockaddr_in6` / `in_addr` / `in6_addr` / `addrinfo`) и какие у них поля. Имена в C/POSIX — сжатые мнемоники из эпохи коротких идентификаторов. **Если знать раскрытие — тип сам себя объясняет, и зоопарк перестаёт быть зоопарком.** Это словарь для накопления (как карточки forensics-инструментов), пополнять по мере встречи.

## Два правила чтения имён

1. **Префикс полей структуры = инициалы типа.** Поля `struct sockaddr` начинаются с `sa_`, `sockaddr_in` — с `sin_`, `sockaddr_in6` — с `sin6_`, `addrinfo` — с `ai_`, `struct stat` — с `st_`. Сделано чтобы поля не конфликтовали с макросами и было видно «откуда поле». Увидел `sin6_scope_id` → это поле `sockaddr_in6`.
2. **`n`/`h` в сетевых функциях — про byte order.** `n` = network (big-endian), `h` = host. `htons` / `ntohs` / `inet_ntop` — читаешь буквы, понимаешь направление.

## Сетевые

| Сокращение | Раскрытие |
|---|---|
| `in` (`sockaddr_in`, `in_addr`, `<netinet/in.h>`) | **In**ternet (IPv4) |
| `in6` (`sockaddr_in6`, `in6_addr`) | **In**ternet v**6** |
| `un` (`sockaddr_un`) | **Un**ix domain socket |
| `sa` (`sa_family`, `struct sockaddr`) | **s**ocket **a**ddress (родовой тип) |
| `sin` (`sin_port`, `sin_addr`) | **s**ockaddr **in**ternet |
| `sin6` (`sin6_port`, `sin6_addr`) | sockaddr internet v6 |
| `ai` (`ai_family`, `ai_addr`, `ai_next`) | **a**ddr**i**nfo (узел списка) |
| `gai` (`gai_strerror`) | **g**et**a**ddr**i**nfo (свой namespace ошибок) |
| `EAI_*` (`EAI_NONAME`, `EAI_AGAIN`) | **E**rror **A**ddr**I**nfo — НЕ из `errno` |
| `s_addr` (в `in_addr`) | **s**truct **addr** — 32-битный IPv4 |
| `s6_addr` (в `in6_addr`) | публичный байтовый вид 128-битного IPv6 |
| `htons`/`ntohs` | **h**ost **to n**etwork **s**hort / network to host short |
| `htonl`/`ntohl` | ... **l**ong (32 бита) |
| `inet_ntop`/`inet_pton` | **n**etwork **to p**resentation (бинарь→текст) / presentation to network |
| `AF`/`PF` | **A**ddress **F**amily / **P**rotocol **F**amily |
| `INADDR_ANY` | **in**ternet **addr**ess: any (wildcard `0.0.0.0`) |

## Зоопарк адресных структур (кто во что вложен)

```
struct sockaddr         ← РОДОВОЙ. У API-функций аргумент этого типа (bind/connect/accept).
                          Поле sa_family говорит, чем он на самом деле является.
   ├─ struct sockaddr_in    ← IPv4-конкретизация (16 байт)
   │     sin_family, sin_port, sin_addr (тип struct in_addr → s_addr: 32 бита)
   ├─ struct sockaddr_in6   ← IPv6-конкретизация (28 байт)
   │     sin6_family, sin6_port, sin6_addr (тип struct in6_addr → 128 бит)
   └─ struct sockaddr_storage ← буфер «под любое семейство» (worst-case) для accept/recvfrom

struct addrinfo         ← узел связного списка из getaddrinfo.
   ai_family/ai_socktype/ai_protocol ← прямо в socket(...)
   ai_addr (struct sockaddr *) + ai_addrlen ← прямо в connect/bind
   ai_next ← следующий узел
```

Логика: **`sockaddr` — generic «как enum базового класса», `sockaddr_in*` — конкретные варианты, `ai_family`/`sa_family` — type tag** по которому кастуешь generic в конкретный. Приём «один буфер + поле-метка + каст» — сквозной в C (см. также union в `in6_addr`).

## `in6_addr` — union из трёх массивов

```c
union { uint8_t addr8[16]; uint16_t addr16[8]; uint32_t addr32[4]; };
```
Все три **наложены на одни и те же 16 байт** (128 бит). Три «масштаба» доступа без копирования: байты (печать/`inet_ntop`), 16-битные группы (текстовый формат `xxxx:xxxx:...`), 32-битные слова (быстрое сравнение/обнуление — 4 операции вместо 16). Имена с `__` — внутренние; публичный фасад `s6_addr` = байтовый вид.

## Системные / общие

| Сокращение | Раскрытие |
|---|---|
| `errno` | **err**or **n**umber |
| `creat`, `O_CREAT`, `O_EXCL`, `O_TRUNC` | **creat**e (без `e` — опечатка Томпсона), **excl**usive, **trunc**ate |
| `stat`/`fstat`/`lstat` | file **stat**us; `f`=по **f**d; `l`=по **l**ink (не разыменовывать symlink) |
| `st_*` (`st_nlink`, `st_mode`) | поля `struct stat`; `nlink` = **n**umber of **link**s |
| `pid`/`ppid`/`tid` | **p**rocess / **p**arent **p**rocess / **t**hread **id** |
| `argv`/`argc` | **arg**ument **v**ector / **c**ount |
| `tv_sec`/`tv_usec` | **t**ime **v**alue; `usec` = **micro**seconds (µ→u) |
| `brk`/`sbrk` | program **br**ea**k** (граница кучи) |

## printf: width vs precision (всплыло в B-4)

| Спека | Смысл |
|---|---|
| `%20s` | **width** (минимум): дополнить пробелами, *никогда не режет*. `%-20s` — влево. |
| `%.20s` | **precision** (максимум): обрезать до 20. **Режет данные.** |

Точка переключает width→precision. Для выравнивания колонки адресов — `%-46s` (`INET6_ADDRSTRLEN`), не `%.20s`.

## Ключевые термины (English)

- **type tag** — поле-метка (`ai_family`/`sa_family`), по которому решают, во что кастовать generic-указатель.
- **generic struct** (`sockaddr`) — родовой тип, «база» для конкретных вариантов.
- **exact-fit vs worst-case** — библиотека выделяет точный буфер (getaddrinfo) vs вызывающий выделяет максимально большой (`sockaddr_storage` для accept/recvfrom).
- **byte order** — `htons`/`ntohs` (`n`=network/big-endian, `h`=host).

## Связанные темы

[[getaddrinfo]] [[tcp-sockets]] [[error-handling-c]]
