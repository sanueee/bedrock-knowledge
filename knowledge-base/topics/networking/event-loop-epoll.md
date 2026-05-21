---
тема: Event loop + epoll / Event-driven server model
блок: B — Сетевой стек
дата: 2026-05-20
связано:
  - "[[tcp-sockets]]"
  - "[[fd-kernel-model]]"
  - "[[redis-anet]]"
---

# Event loop + epoll / Event-driven server model

## Что это

Архитектура сервера, в которой **один поток** обслуживает тысячи соединений через **системный механизм мультиплексирования** (epoll на Linux, kqueue на BSD, IOCP на Windows). Альтернатива thread-per-connection и process-per-connection. Используется в Redis, nginx, Node.js, Tokio, всех современных high-load сервисах.

## Ключевые термины (English)

- **event loop** — главный цикл, в котором поток `epoll_wait`'ит, разбирает события и обрабатывает их.
- **multiplexing** — обслуживание нескольких I/O потоков одним thread'ом через один блокирующий вызов.
- **readiness model** — ядро говорит "fd готов к операции", приложение само выполняет syscall. Альтернатива — **completion model** (Windows IOCP, Linux io_uring), где ядро само выполняет I/O и сигналит "сделано".
- **non-blocking I/O** — `read()`/`write()` не зависают: если данных нет — возвращают `-1` + `errno == EAGAIN` (или `EWOULDBLOCK`, синонимы).
- **level-triggered (LT)** — epoll по умолчанию: пока fd готов, событие приходит снова и снова.
- **edge-triggered (ET)** — `EPOLLET`: событие приходит **один раз** при переходе fd из not-ready в ready. Требует читать до `EAGAIN`, иначе зависнешь.
- **back-pressure** — клиент пишет быстрее чем сервер читает: send buffer заполняется, `write()` возвращает `EAGAIN`. Корректная реакция — переподписаться на `EPOLLOUT`, дописать когда место появится.

## Три модели обслуживания N клиентов

### Модель A — thread-per-connection

```c
while (1) {
    int cfd = accept(listen_fd, ...);
    pthread_create(&t, NULL, handle_client, &cfd);  // поток на каждого
}
```
- **Цена:** 10k клиентов × 8 МБ стек = 80 ГБ. Plus context switches.
- **Когда применимо:** до сотен соединений.

### Модель B — process-per-connection (старый prefork Apache)

`fork()` на каждого клиента. Ещё дороже B (полный address space copy через COW, потом расхождения).

### Модель C — однопоточный event loop

```c
int epfd = epoll_create1(0);
epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev);
fcntl(listen_fd, F_SETFL, O_NONBLOCK);

while (1) {
    int n = epoll_wait(epfd, events, MAX, -1);   // блокирующий — спим
    for (int i = 0; i < n; i++) {
        if (events[i].data.fd == listen_fd) accept_new();
        else handle_io(events[i].data.fd);         // read/write non-blocking
    }
}
```

100k клиентов = **тот же один поток**. Память — только на буферы соединений, не на стеки.

## Почему `O_NONBLOCK` обязателен в event loop

Без него один медленный клиент остановит весь сервер. Сценарий:
- epoll сказал "fd #5 readable, можно читать".
- ты делаешь `read(5, buf, 4096)` — получаешь 3 байта (всё что было).
- зовёшь снова — а данных больше нет → **`read` зависает**.
- весь сервер встал, никто из других тысяч клиентов не обслуживается.

С `O_NONBLOCK`: второй `read` вернёт `-1` + `EAGAIN`. Идиома цикла:
```c
while (1) {
    ssize_t r = read(fd, buf, sizeof(buf));
    if (r > 0)        { process(buf, r); continue; }
    if (r == 0)        { close_connection(); break; }    // EOF
    if (errno == EINTR) continue;
    if (errno == EAGAIN) break;                          // данных пока нет — выйти
    /* настоящая ошибка */
}
```

## Readiness ≠ correctness

Когда epoll говорит "writable" — это **не** "у тебя всё хорошо". Это "ядро тебя будит, проверь сам".

Сценарий non-blocking `connect()`:
- `connect()` вернул `-1` + `EINPROGRESS` — handshake в фоне.
- Регистрируешь fd в epoll на `EPOLLOUT`, засыпаешь в `epoll_wait`.
- Handshake **проваливается** (RST или таймаут). Соединения нет, но **что ядру делать со спящим процессом?** Не будить его — баг (висишь вечно). Будит через тот же сигнал "writable", потому что в базовой readiness-модели нет отдельного "errored". Ты проверяешь через `getsockopt(SO_ERROR)`, видишь `ECONNREFUSED`.

Принцип: **ядро гарантирует разбудить, не гарантирует что состояние хорошее**. Всегда проверяй результат конкретного syscall (`read`/`write`/`getsockopt`).

## Подводные камни

- **Длинные операции блокируют всех.** В Redis `KEYS *` на миллион ключей — секунды block'а на CPU, в это время остальные клиенты висят. Поэтому архитектурно: операции в event loop должны быть **короткие**. Тяжёлое — в отдельные потоки (Redis 6+ I/O threading, lazy free) или асинхронные таски.
- **Forgotten EAGAIN handling.** Если на write не обработать EAGAIN с переподпиской на EPOLLOUT — данные потеряются (или хуже, при retry будут поверх старых).
- **Spurious wakeups в level-triggered.** Поток просыпается, fd "readable", но `read` возвращает `EAGAIN` (race: другой поток успел прочитать, либо ядро было оптимистично). Лечится `if (errno == EAGAIN) continue;`.
- **Edge-triggered + неполное чтение = зависание.** Если в ET-режиме прочитал 100 байт из 200 и не дочитал до `EAGAIN` — больше события на этот fd не придёт, оставшиеся 100 байт зависнут навсегда.
- **One thread = no parallelism on CPU.** Event loop хорош для I/O-bound нагрузок. Для CPU-bound (хеширование, шифрование больших блоков) нужен отдельный thread pool.

## Связанные темы

[[tcp-sockets]] [[fd-kernel-model]] [[redis-anet]] [[async-signal-safe]]
