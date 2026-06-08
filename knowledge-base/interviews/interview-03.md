# Interview 03 — 2026-06-09

Scope: блок B (B-1…B-6), проверка удержания. 6 вопросов.

## Темы
- B-2 — TCP-сокеты: три исхода `recv` (`>0`/`0`/`-1`).
- B-3 — pcap/парсинг: переменная длина IP-заголовка (`ip_hl × 4`).
- B-4 — getaddrinfo: извлечение адреса из `struct sockaddr` кастом по `ai_family`.
- B-5 — host-probe: `recv == -1` — `EAGAIN` (таймаут `SO_RCVTIMEO`) vs `EINTR` (сигнал).
- B-6 — non-blocking: `getsockopt(SO_ERROR)` после `poll(POLLOUT)` (readiness ≠ correctness); blocking vs non-blocking + роль `poll`.

## Уверенно знает
- `recv == 0` = orderly shutdown (FIN, EOF на потоке); отличает от `-1`.
- `ip_hl × 4` = длина IP-заголовка; поле 4-битное в 32-битных словах, self-describing формат (та же логика `th_off × 4`).
- `struct sockaddr` — generic база; `ai_family` = type tag (discriminated union); каст к `sockaddr_in`/`sockaddr_in6` выбирает **схему чтения**, не двигает байты.
- `EAGAIN` vs `EINTR` на `recv`: противоположные реакции (дедлайн истёк → сдаться vs помеха → retry); идиома «съесть EINTR в do/while».
- **B-6 readiness ≠ correctness** — разобрал отлично: `POLLOUT` срабатывает и на успех, и на провал handshake (RST → `ECONNREFUSED`); в readiness-модели нет отдельного «errored»-сигнала; ошибка асинхронна → `errno` пуст → припаркована на сокете → `getsockopt(SO_ERROR)`, read-and-clear.

## Нужно повторить
- **Что делает сокет блокирующим.** Путаница: «сокет блокирующий, потому что мы поставили таймаут». На деле blocking — **режим по умолчанию** (с `socket()`), ничего ставить не надо; `SO_RCVTIMEO` лишь **ограничивает уже существующую** блокировку. См. [[nonblocking-poll]] → «Как работает: три роли» + таблица источников флага (`fcntl O_NONBLOCK` vs `setsockopt SO_RCVTIMEO`).
- **Что определяет исход «hang vs мгновенная сдача».** На вопрос «убрал `poll(POLLIN)` — программа зависнет?» ответил «может случиться что угодно, либо зависнем либо выйдем». На деле исход **детерминирован режимом**: non-blocking → `recv` мгновенно `EAGAIN` → выход без баннера (**не** hang). Hang — свойство blocking без таймаута. Матрица 2×2 ([[nonblocking-poll]]) ещё не автоматизм.
- Терминология: `EINTR` = прерван **сигналом** (signal), не «сисколлом» (`recv` сам и есть syscall).

## NotebookLM Prompt
---
Я изучаю системное программирование на C под Linux (сетевой стек, сокеты). Вот что я уже прошёл:
TCP-сокеты (socket/bind/listen/accept/connect/send/recv, три исхода recv, partial read, SO_REUSEADDR/TIME_WAIT, EPIPE/SIGPIPE), парсинг пакетов (Ethernet/IPv4/TCP, переменная длина заголовков ip_hl*4/th_off*4, network byte order), getaddrinfo (addrinfo linked list, sockaddr cast по ai_family, gai_strerror), read timeout SO_RCVTIMEO + различение EAGAIN/EINTR, non-blocking I/O (O_NONBLOCK, EINPROGRESS на connect, poll(POLLOUT/POLLIN), getsockopt(SO_ERROR)).

На мок-собесе я показал уверенные знания по:
исходам recv, переменной длине IP/TCP заголовков, кастам sockaddr по ai_family, различению EAGAIN vs EINTR, проверке SO_ERROR после неблокирующего connect (readiness != correctness).

Но у меня есть пробелы в следующих темах:
1. Что именно делает сокет блокирующим. Я путаю: думаю, что таймаут (SO_RCVTIMEO) делает сокет блокирующим. На деле blocking — режим по умолчанию, а SO_RCVTIMEO лишь ограничивает уже существующую блокировку. Не держу в голове, что blocking/non-blocking задаётся флагом O_NONBLOCK через fcntl, а не таймаутом.
2. Что определяет, зависнет программа или мгновенно сдастся, если убрать poll перед recv. Я считаю это недетерминированным («что угодно»), хотя исход жёстко задаётся режимом сокета: на non-blocking recv возвращает EAGAIN мгновенно (выход без данных, не hang); hang бывает только на blocking без таймаута.

Пожалуйста, сфокусируй объяснение на слабых местах. Для каждого пробела:
1. Объясни концепцию точно и без упрощений
2. Покажи минимальный пример кода
3. Задай мне один проверочный вопрос по этой теме
---

## Закрепляющее задание
[[task-b-interview03-reinforce]] — `sockmode_demo`: один и тот же `recv` на молчащем сервере в трёх конфигурациях (blocking+SO_RCVTIMEO / non-blocking / blocking без таймаута — последний под `alarm`), замер времени, чтобы **физически** почувствовать матрицу 2×2. Закрыть до перехода к B-7 (вместе с `/theory` по [[nonblocking-poll]]).

## Связанные темы
[[nonblocking-poll]] [[tcp-sockets]] [[getaddrinfo]] [[event-loop-epoll]]
