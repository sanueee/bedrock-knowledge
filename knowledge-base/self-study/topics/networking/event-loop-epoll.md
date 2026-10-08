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

## Реализация: epoll_echo.c (B-7, 2026-06-18)

Однопоточный многоклиентский echo на LT. Узлы, которые реально пришлось собрать руками (по словам — логика сервера новая целиком + большой пласт нового синтаксиса):

### Диспетчер по битовой маске `events[i].events`

«Пришло событие» ≠ «надо читать». После `epoll_wait` на каждом fd проверяем биты:
```c
uint32_t e = events[i].events;
if (e & (EPOLLERR | EPOLLHUP)) drop = 1;          // ядро сообщило об обрыве
if (!drop && (e & EPOLLOUT))  flush_out(...);      // готов на запись — дослать остаток
if (!drop && (e & EPOLLIN))   recv + echo;         // есть входные данные
```
Один fd за оборот может иметь сразу `EPOLLIN|EPOLLOUT` — обрабатываются обе ветки.

### EPOLLOUT-backlog: что делать с недописанным `send`

На non-blocking сокете `send` может записать **частично** или вернуть `EAGAIN` (буфер отправки ядра полон). Крутить `send` на месте = busy-wait (буфер не освободится, пока сеть не вычитает; весь loop висит). Правильно — **отдать решение «когда повторить» epoll'у**:
- непосланный остаток сложить в per-client буфер (`conn_t { buf; len; sent; }`);
- `arm(fd, EPOLLIN|EPOLLOUT)` — попросить разбудить, когда сокет снова writable;
- по `EPOLLOUT` дослать (`flush_out`); опустошил backlog → `arm(fd, EPOLLIN)`, снять `EPOLLOUT`.

`arm()` — обёртка над `epoll_ctl(EPOLL_CTL_MOD, ...)`.

### Flow-control размен (LT)

Backlog мог бы расти без границы под медленного клиента. Решение: **читать новую порцию только когда прошлая отправлена целиком** (`if (c->len == 0) recv(...)`). Платой идёт узел «drain recv до EAGAIN» — здесь читаем по одной порции за оборот, а недочитанное в LT само поднимет `EPOLLIN` снова. Осознанный размен: ограниченность памяти против пропускной способности.

### Снятие клиента и reuse fd

- `EPOLL_CTL_DEL` принимает `NULL` вместо `&ev` (с Linux 2.6.9) — событие при удалении не нужно.
- Порядок строго `EPOLL_CTL_DEL` → `close(fd)`. Закрыть раньше DEL — дыра: номер fd может переиспользоваться, и DEL уйдёт уже по чужому fd.
- `conns[fd]` индексируется **номером fd** (статический массив, guard `cfd >= FD_MAX`). При закрытии — `conns[fd].len = sent = 0`: иначе остаток старого клиента «протёк» бы в нового с тем же номером fd (stale state).

### Чистый выход по SIGINT (sigaction без SA_RESTART)

Обработчик только взводит флаг `static volatile sig_atomic_t g_stop` (в хендлере — больше ничего, async-signal-safety). Установка через `sigaction` **без `SA_RESTART`**: тогда Ctrl+C прерывает `epoll_wait` с `EINTR`, главный цикл (`while (!g_stop)`) видит флаг и выходит на `close(listen_fd)/close(epfd)`. С `SA_RESTART` syscall перезапустился бы сам и флаг бы не проверился. Уборку делает цикл, не хендлер.

> Прогон (Linux-контейнер, т.к. epoll Linux-only — на macOS не собрать): сборка чистая
> `-Wall -Wextra`, 3 одновременных клиента получили свой echo независимо. EPOLLOUT-путь и
> SIGINT-выход разобраны логически (автотест уперся в собственный deadlock write-then-read).

## Связанные темы

[[tcp-sockets]] [[fd-kernel-model]] [[redis-anet]] [[async-signal-safe]] [[nonblocking-poll]]
