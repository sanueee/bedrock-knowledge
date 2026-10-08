---
type: atomic
block-position: B-17
phase: 4
title: Raw sockets и capabilities — CAP_NET_RAW, minimum capability surface
status: выполнено
date: 2026-07-20
tags: [networking, capabilities, raw-socket, cap_net_raw, libcap-ng, least-privilege]
---

# B-17 — Raw sockets и capabilities

**Тип:** атомарное (позиция 17/26, Фаза 4 — Raw sockets + L2/L3).
**Синтез опоры:** сокеты и `socket()` (B-2), привилегии на захват трафика (B-3 pcap-sniffer — там уже упирался в root), обработка `errno`/`EPERM`.
**Готовит:** фундамент под B-18 (checksum) → B-19 (ICMP echo через raw socket) → B-20 (ping sweep). Начиная с B-19 весь код требует `CAP_NET_RAW` — модель привилегий надо понять сейчас, до первого реального пакета.

Перечитать перед стартом: конспекта по capabilities пока нет — заведём после сессии. Опорная концепция из `topics/networking/libpcap.md` (захват пакетов тоже требует привилегий).
Опора в репо: `self-practice/networking/pcap-sniffer/sniffer.c` (работа с сырым трафиком под привилегиями), любой `socket()`-код (`host-probe/host_probe.c`).

> Тестировать **в Linux** (Docker/Kali VM). На macOS нет `SOCK_RAW` в этом виде, нет `setcap`, нет модели Linux-capabilities. Собирать и гонять внутри Linux-контейнера.

---

## Задание

Написать `self-practice/networking/raw-socket-caps/rawcap.c` — маленькую утилиту, которая
демонстрирует **модель привилегий raw-сокета**: открывает `SOCK_RAW`, показывает свои
capabilities до и после, и **сбрасывает `CAP_NET_RAW` сразу после открытия сокета** —
живой пример принципа *minimum capability surface*.

Утилита **не строит и не шлёт ICMP-пакет** — это темы B-18/B-19. Здесь фокус ровно один:
кто имеет право открыть raw socket, и как держать эту привилегию минимально короткое время.

```
usage: ./rawcap

$ ./rawcap                       # без привилегий
[caps] CAP_NET_RAW effective? NO
[sock] socket(AF_INET, SOCK_RAW, IPPROTO_ICMP): Operation not permitted (EPERM)
       подсказка: sudo setcap cap_net_raw+ep ./rawcap   (или запуск под root)

$ sudo setcap cap_net_raw+ep ./rawcap && ./rawcap    # точечная capability, без root
[caps] uid=1000 euid=1000  CAP_NET_RAW effective? YES
[sock] raw socket opened, fd=3
[drop] cleared all capabilities
[caps] CAP_NET_RAW effective? NO
[test] fd=3 всё ещё валиден (capability нужна была только на socket())
[test] повторный socket(SOCK_RAW): Operation not permitted (EPERM)  ← привилегия ушла, а старый fd жив
```

### Центральная идея — capability нужна на syscall, не на весь процесс

`CAP_NET_RAW` проверяется **в момент вызова** `socket(..., SOCK_RAW, ...)`. Как только
дескриптор выдан — ядро больше не перепроверяет привилегию для этого fd. Значит правильный
паттерн: **захватить привилегию → сделать привилегированный syscall → немедленно сбросить
привилегию**. Дальше процесс работает с уже открытым fd, но новый raw socket открыть уже
не может. Если после этого в процессе найдут дыру (RCE) — атакующему `CAP_NET_RAW` уже
не достанется.

Это и есть *minimum capability surface*: окно, в котором процесс держит опасное право,
сжато до одного вызова.

### Три способа дать право (понять разницу)

| Способ | Что получает процесс | Оценка |
|--------|----------------------|--------|
| `sudo ./rawcap` (root) | **все** capabilities, uid=0 | грубо: даёшь всё ради одного права |
| `setcap cap_net_raw+ep ./rawcap` | **только** `CAP_NET_RAW`, uid обычный | точечно, least privilege ✅ |
| setuid-root бинарник (`chmod u+s`) | euid=0 → снова весь root | ещё хуже setcap, лишний эскалатор |

`+ep` = capability кладётся в наборы **e**ffective + **p**ermitted файла (не inheritable →
не протекает в дочерние процессы). Это и есть целевой способ выдачи для сетевых инструментов.

### Хедеры

```c
#include <stdio.h>       // printf, fprintf, perror
#include <stdlib.h>      // exit, EXIT_FAILURE
#include <unistd.h>      // close, getuid, geteuid
#include <errno.h>       // errno, EPERM
#include <string.h>      // strerror
#include <sys/socket.h>  // socket, AF_INET, SOCK_RAW
#include <netinet/in.h>  // IPPROTO_ICMP
#include <cap-ng.h>      // capng_* (libcap-ng)
```

Сборка: `cc -Wall -Wextra rawcap.c -lcap-ng -o rawcap`
(в Debian/Ubuntu-контейнере нужен пакет `libcap-ng-dev`.)

### libcap-ng — минимальный набор вызовов (посмотри `man 3 capng_*`)

Не готовый код — ориентир, какие функции существуют. Сам собери порядок:

- `capng_get_caps_process()` — загрузить capabilities текущего процесса в рабочий буфер библиотеки.
- `capng_have_capability(CAPNG_EFFECTIVE, CAP_NET_RAW)` — есть ли право в effective-наборе (0/1).
- `capng_clear(CAPNG_SELECT_BOTH)` — обнулить рабочий буфер (все наборы).
- `capng_apply(CAPNG_SELECT_BOTH)` — **применить** рабочий буфер к процессу (до этого — только в памяти библиотеки, на процесс не влияет).

Ключ: `capng_clear` меняет буфер, `capng_apply` — реально роняет права процесса. Между ними
ничего не «сброшено» по-настоящему.

### Декомпозиция — выведи сам, разберём по одной

Функций тут мало (утилита почти линейная). Прежде чем писать, ответь себе:

- Печать «есть ли CAP_NET_RAW» повторяется до и после drop — это отдельная функция или два
  инлайн-вызова? Если функция — что она принимает, что возвращает (или просто печатает)?
- Что нужно сделать **до** первого запроса capability — загрузить их (`capng_get_caps_process`)?
  Что будет, если спросить `capng_have_capability` без загрузки?
- Порядок в `main`: сначала проверить право, потом `socket()` — или наоборот, `socket()`
  и по `EPERM` уже объяснять? Что нагляднее для демонстрации?
- Сброс: `capng_clear` затем `capng_apply` — в каком порядке и что доказывает повторный
  `socket()` после них?

Предложи свой список (имя + что делает + вход/выход) — разберём. Каркас `main` уже в `.c`.

### Алгоритм (пошагово, с вызовами)

1. Читает свой uid/euid для контекста вывода: `getuid()`, `geteuid()`.
2. Загружает свои capabilities и проверяет наличие `CAP_NET_RAW`:
   `capng_get_caps_process()`, затем `capng_have_capability(CAPNG_EFFECTIVE, CAP_NET_RAW)`. Печатает YES/NO.
3. Открывает raw socket: `socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)`.
   При `-1` — по `errno` (`EPERM`) печатает понятную ошибку + подсказку про `setcap`, `exit`.
4. Открылось — печатает `fd`.
5. Сбрасывает все capabilities: `capng_clear(CAPNG_SELECT_BOTH)`, затем `capng_apply(CAPNG_SELECT_BOTH)`.
6. Повторно проверяет `CAP_NET_RAW` — теперь NO (перезагрузить состояние `capng_get_caps_process()` перед проверкой).
7. Доказывает, что старый fd жив, а новое право ушло: печатает, что `fd` валиден, и делает
   **второй** `socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)` — ждём `EPERM` (привилегия сброшена).
8. `close(fd)` (и закрыть второй fd, если вдруг открылся).

## Примеры для теста (в Linux-контейнере)

- **Тест 1 — без права** — `./rawcap` от обычного пользователя → шаг 3 даёт `EPERM`, видна подсказка.
- **Тест 2 — под root** — `sudo ./rawcap` → сокет открылся, drop сработал, повторный `socket()` → `EPERM`.
- **Тест 3 — точечная capability** — `sudo setcap cap_net_raw+ep ./rawcap` затем `./rawcap` **без** sudo →
  работает как обычный пользователь (uid≠0), но право есть. Сверить: `getcap ./rawcap`, `capsh --print` / `getpcaps $$`.
- **Тест 4 — наблюдение** — снаружи посмотреть на процесс: `cat /proc/<pid>/status | grep Cap` (поля `CapEff`/`CapPrm`).

---

## Разбор /check (3 итерации + прогон)

| Итерация | Найдено | Класс |
|----------|---------|-------|
| 1 | перевёрнута логика второго `socket()` (EPERM → `exit(FAILURE)`, хотя это успех демонстрации); fd первого сокета не закрывался; `capng_apply` возврат не проверен; проверка capability только ветка `NO` | crit |
| 2 | ветка «второй `socket()` **открылся**» не обработана → провал minimum surface проглатывается молча + `new_fd` leak; `EPERM`-вывод оформлен как ошибка с подсказкой `setcap` (мы сами сбросили право) | crit |
| 3 | функционально чисто; косметика: `uid_t` через `%d`, backslash-continuation втягивает отступ, порядок stdout/stderr | fixed |
| прогон (Docker) | **Тест 3 (setcap + nobody) упал: `error: capng_apply`** — `CAPNG_SELECT_BOTH` трогает bounding set, сброс требует `CAP_SETPCAP`, которого нет у non-root с одной cap → замена на `CAPNG_SELECT_CAPS` | crit (нашёл тест) |

**Ключевой урок процесса:** главный баг (`_BOTH` vs `_CAPS`) поймал **только прогон целевого сценария** — по чтению `_BOTH` выглядел «полнее и безопаснее», а именно он рушил ровно тот путь (точечная capability под non-root), ради которого писалась утилита. Тест ловит то, что чтение не видит. Concept-стыки (проверка права в момент syscall, старый fd переживает drop) усвоены на `/theory` до кода и не ломались.

## Security-заметки
- **raw socket = мощное право** (спуфинг source IP → reflection/amplification-DoS, сниффинг, инъекция) → gated `CAP_NET_RAW`, не доступно обычному процессу.
- **minimum capability surface**: `socket()` → сразу `capng_clear + apply`; окно с опасным правом сжато до одной строки, RCE после сброса не даёт `CAP_NET_RAW`.
- **`+ep` vs setuid-root**: точечная capability, не весь root; компрометация не даёт чтения `/etc/shadow` / `kill` / mount. `e`-флаг не inheritable → не протекает в дочерние.
- **`capng_clear` — `void`, только буфер**; без `capng_apply` сброс фиктивный (второй `socket()` открылся бы) — guard на возврат `apply` обязателен.
- **`CAPNG_SELECT_BOTH` требует `CAP_SETPCAP`** (сброс bounding set) — для сброса самого права брать `CAPNG_SELECT_CAPS`.

## Session-debrief / итог

**Что усвоено:** модель capabilities (permitted=склад / effective=в руках / inheritable; ядро проверяет effective в момент syscall, один раз → fd переживает drop); паттерн захватил→syscall→сбросил; три способа выдачи права (root / setuid-root / `setcap +ep`) и почему `setcap` = least privilege; libcap-ng (буфер библиотеки vs процесс, `clear` void → `apply` коммитит); `_BOTH` vs `_CAPS` через `CAP_SETPCAP`.

**Трудность (сам сформулировал):** механика простая, но **самому вывести какие сценарии привилегий бывают и какие проверки нужны на каждом** — было бы тяжело. Это подтверждает известный паттерн: концепт-стыки усваиваются, а перебор ветвей/проверок нагружает (см. [[feedback-branch-fatigue-mechanical-slips]]). Три ветки (nobody/root/setcap) выведены в диалоге `/theory`, не самостоятельно.

**Слабое место:** перебор сценариев и синхронизация «на каждой ветке — своя проверка/трактовка исхода» — как и на B-13/B-15, механическая нагрузка, не непонимание. Прогон обязателен — Тест 3 поймал баг, невидимый по чтению.

## Ключевые термины (English)
- **capability** — бит привилегии из разбитого root (`CAP_NET_RAW`, `CAP_SETPCAP`).
- **permitted / effective / inheritable set** — наборы прав процесса; effective проверяется при syscall.
- **file capabilities** — xattr `security.capability` на бинарнике (`setcap cap_net_raw+ep`).
- **least privilege / minimum capability surface** — минимум прав минимальное время.
- **raw socket** — `SOCK_RAW`, приложение строит заголовки → gated `CAP_NET_RAW`.
