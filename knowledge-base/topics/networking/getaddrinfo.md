---
тема: getaddrinfo и addrinfo linked list
блок: B — Сетевой стек
дата: 2026-05-20
связано:
  - "[[tcp-sockets]]"
  - "[[redis-anet]]"
---

# getaddrinfo и addrinfo linked list

## Что это

Современный POSIX API для resolve "имя + порт → список адресов для подключения". Заменяет связку `gethostbyname` + ручной `sockaddr_in`. Возвращает **связный список** `struct addrinfo`, потому что одно имя может соответствовать нескольким адресам (IPv4 + IPv6, ферма серверов).

## Ключевые термины (English)

- **resolve** — преобразовать имя в адрес. Шаги: `/etc/nsswitch.conf` → `/etc/hosts` или DNS (`/etc/resolv.conf` для серверов).
- **addrinfo** — структура с готовым `sockaddr` для bind/connect плюс ai_family/socktype/protocol.
- **AI_PASSIVE** — флаг для серверной стороны: если `host == NULL`, поставить wildcard (`INADDR_ANY` / `in6addr_any`) — слушать на всех интерфейсах.
- **AI_NUMERICHOST** — не ходить в DNS, требовать чтобы host был уже IP-литералом.
- **gai_strerror** — расшифровщик кодов ошибок (свой namespace, не errno).
- **EAI_NONAME**, **EAI_AGAIN**, **EAI_FAMILY** — типичные коды: "имени нет", "временная ошибка DNS, повтори", "семейство не поддерживается".
- **RFC 6724** — алгоритм сортировки адресов, который ядро/glibc применяют (предпочесть IPv6, совпадающую scope и т.д.) — поэтому первый успешный = "лучший доступный".

## Как работает

```c
struct addrinfo hints, *info;
memset(&hints, 0, sizeof(hints));
hints.ai_family = AF_UNSPEC;       // и v4, и v6
hints.ai_socktype = SOCK_STREAM;   // только TCP-варианты
int rv = getaddrinfo("redis.example.com", "6379", &hints, &info);
if (rv != 0) {
    fprintf(stderr, "%s\n", gai_strerror(rv));  // НЕ strerror(errno)
    return -1;
}
```

Результат — связный список:
```
info → {ai_family=AF_INET6, ai_addr=...} → {AF_INET6, ...} → {AF_INET, ...} → NULL
```

Итерация:
```c
for (p = info; p != NULL; p = p->ai_next) {
    s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (s == -1) continue;
    if (connect(s, p->ai_addr, p->ai_addrlen) == 0) break;  // удача
    close(s);                                                // эта попытка не пошла
}
if (p == NULL) { /* все варианты исчерпаны */ }
freeaddrinfo(info);    // обязательно
```

Структура `addrinfo`:
```c
struct addrinfo {
    int              ai_flags;
    int              ai_family;     // AF_INET / AF_INET6
    int              ai_socktype;   // SOCK_STREAM / SOCK_DGRAM
    int              ai_protocol;
    socklen_t        ai_addrlen;
    struct sockaddr *ai_addr;       // готов к bind/connect
    char            *ai_canonname;
    struct addrinfo *ai_next;
};
```

Главное удобство — `ai_addr` готов, не надо вручную набивать `sockaddr_in.sin_family`/`sin_port`/`sin_addr` и беспокоиться про v4 vs v6.

## Свой namespace ошибок + своя strerror (важный паттерн)

`getaddrinfo` **не выставляет `errno`**, а возвращает код ошибки напрямую (`EAI_NONAME`, `EAI_AGAIN`, ...). Эти коды НЕ из пространства `errno` — `EAI_NONAME == -2` в glibc, и это пересекается с какой-нибудь `ENOENT` совершенно другого смысла. `strerror(EAI_NONAME)` вернёт мусор или неверный текст.

**Почему так сделали:** `getaddrinfo` внутри делает кучу системных вызовов (открыть `/etc/hosts`, DNS-запросы, парсинг ответа). Каждый из них может выставить свой `errno`. К моменту возврата `errno` отражает причину **последней** системной операции — не обязательно главной. Поэтому отдельный namespace кодов + своя функция `gai_strerror`.

Паттерн: **у библиотеки свои коды и своя расшифровка**. Встретится везде где библиотечный API делает многошаговую работу — OpenSSL (`ERR_get_error` + `ERR_error_string`), libcurl (`CURLcode` + `curl_easy_strerror`), zlib, и т.д.

## Подводные камни

- **Забыл `freeaddrinfo`** → утечка памяти. Список аллоцирован библиотекой.
- **`hints` не занулил** через `memset` → мусор в полях → странные результаты.
- **`AI_PASSIVE` без `host == NULL`** не делает ничего полезного. Серверная сторона: `getaddrinfo(NULL, "6379", &hints, ...)` + `AI_PASSIVE` → wildcard.
- **Не итерировал по `ai_next`** — взял первый, не сработал, сдался. На IPv6-only ноде нужен fallback на v4 (или наоборот). См. как Redis делает в `_anetTcpServer` — пробует все.
- **`strerror(errno)` после `getaddrinfo`** — частая ошибка. Только `gai_strerror(rv)`.

## Связанные темы

[[tcp-sockets]] [[redis-anet]]
