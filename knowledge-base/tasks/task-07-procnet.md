---
task: task-07
title: procnet — чтение активных TCP-соединений из /proc/net/tcp
status: in-progress
topic: "[[proc-net]]"
---

## Задание

Написать программу `procnet` на C.

**Что делает:**
Читает `/proc/net/tcp`, парсит hex-адреса и порты, выводит список активных TCP-соединений в читаемом виде.

**Формат вывода:**
```
local           remote          state
127.0.0.1:8080  0.0.0.0:0       LISTEN
127.0.0.1:52341 93.184.216.34:443  ESTABLISHED
```

**Детали реализации:**
1. Открыть `/proc/net/tcp` через `fopen()`
2. Пропустить первую строку (заголовок) через `fgets()`
3. Читать строки в цикле через `fgets()`
4. Из каждой строки распарсить:
   - local address: поле 2 (`XXXXXXXX:PPPP` — hex IP:hex port)
   - remote address: поле 3
   - state: поле 4 (hex число: `01` = ESTABLISHED, `0A` = LISTEN)
5. Конвертировать hex IP в dotted decimal (`strtol` + побайтовый разбор)
6. Конвертировать hex port в десятичный
7. Вывести только соединения в состоянии ESTABLISHED и LISTEN

**Функции для использования:**
- `fopen()`, `fgets()`, `fclose()` — чтение файла
- `sscanf()` — парсинг строки по формату
- `strtol(str, NULL, 16)` — hex строка → число
- побитовые операции для разбора IP: `(ip >> 24) & 0xFF` и т.д.

**Где сохранить:** `linux/proc/procnet.c`

**Как проверить:**
```
gcc procnet.c -o procnet && ./procnet
```
Сравнить с выводом `ss -tn` или `netstat -tn`.

**Vault после завершения:** `knowledge-base/topics/linux/proc-net.md`

---

## Что узнал

*(заполняется после session-debrief)*

## Сложности

*(заполняется после session-debrief)*

## Код

*(ссылка на файл после выполнения)*
