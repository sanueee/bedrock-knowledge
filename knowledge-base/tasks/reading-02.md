---
task: reading-02
title: Redis anet.c — обёртка над sockets API
status: выполнено
date: 2026-05-21
type: reading
---

## Источник
- Проект: Redis
- Файл: `src/anet.c` (upstream: https://github.com/redis/redis/blob/unstable/src/anet.c)
- Версия / commit: unstable (или последний тег 7.x — API anet стабилен годами)

## Цель чтения
**Как Redis превращает голый sockets API (socket/bind/listen/accept/connect) в безопасную обёртку `anetTcp*`, и какие классы ошибок этот слой скрывает от вызывающего кода?**

Три подвопроса для фокуса:
1. **Управление ошибками библиотечного слоя.** Что делает `anetSetError`, зачем `err`-буфер вместо чтения `errno` вызывающим кодом? Сравнить с моим `echo_server.c`, где я звал `perror`+`exit(1)`.
2. **Гигиена сокета по умолчанию.** Какие `setsockopt` Redis ставит автоматически (`SO_REUSEADDR`, `O_NONBLOCK`, `TCP_NODELAY`) и зачем — что из этого я пропустил в `echo_server.c`?
3. **Симметрия `connect()` vs `bind()/listen()`.** Общая структура (resolve → socket → действие → cleanup). Главное: кто закрывает `fd` если socket() ок, но connect()/bind() упал — какой паттерн используется для cleanup на ошибке посередине?

## Что прочитать
Функции (~250 строк):
- `anetSetError` (helper для записи в err-буфер)
- `anetSetBlock` / `anetNonBlock` / `anetBlock` (управление O_NONBLOCK через fcntl)
- `anetSetReuseAddr`
- `anetTcpGenericConnect` (главная обёртка connect)
- `_anetTcpServer` + `anetTcpServer` (главная обёртка bind+listen)
- `anetTcpAccept` / `anetGenericAccept`

Опционально посмотреть рядом: `anetEnableTcpNoDelay`, `anetKeepAlive`.

Связанная теория (перечитать перед началом):
- [[tcp-sockets]] — socket/bind/listen/accept/connect, SO_REUSEADDR, partial read
- [[error-handling-c]] — errno, perror

---

## Что понял

Главное — итерация по списку `addrinfo`, симметрия connect/server вокруг "resolve → попробовать каждый кандидат → cleanup", библиотечная дисциплина (`anetSetError` вместо `perror`), `goto error` как единая точка cleanup в C.

Подробности — в конспекте [[redis-anet]].

## Что осталось непонятным

- `IPV6_V6ONLY` — почему IPv6-сокет по умолчанию принимает v4-маппинг и когда это включать.
- `anetPipe` с веткой `pipe2` — стоит вернуться после задачи на pipe2/CLOEXEC.
- `anetAcceptFailureNeedsRetry` — детально каждый errno в списке.

## Интересные приёмы

См. [[redis-anet]] раздел "Интересные приёмы" — особенно паттерн "своё пространство ошибок + своя strerror" ([[getaddrinfo]]).

## Связанные темы

[[redis-anet]] [[event-loop-epoll]] [[nagle-tcp-nodelay]] [[getaddrinfo]] [[bitwise-flags]] [[tcp-sockets]]
