# anon_inode — анонимные файловые дескрипторы

## Суть (своими словами)

Некоторые fd не указывают на файл или сокет. Они указывают на специальный механизм ядра у которого нет пути в файловой системе. Такие fd показываются как `anon_inode:[тип]`.

Увидел это в выводе fdlist на systemd-journal:
```
fd 7  -> anon_inode:[eventpoll]
fd 11 -> anon_inode:[signalfd]
fd 16 -> anon_inode:[timerfd]
```

## Три основных типа

**`anon_inode:[eventpoll]`** — epoll.
Механизм для слежения за множеством fd одновременно. Процесс говорит ядру: "скажи мне когда хоть один из этих 30 сокетов станет готов к чтению". Ядро сообщает одним событием. Без этого пришлось бы опрашивать каждый fd по очереди.

```c
int epfd = epoll_create1(0);  // возвращает fd типа eventpoll
```

**`anon_inode:[signalfd]`** — сигналы как fd.
Обычно сигналы (SIGINT, SIGTERM и др.) приходят асинхронно через обработчики. `signalfd` позволяет читать их через обычный `read()` — как данные из файла. Удобно комбинировать с epoll.

```c
int sfd = signalfd(-1, &mask, 0);  // возвращает fd типа signalfd
```

**`anon_inode:[timerfd]`** — таймер как fd.
Таймер который "срабатывает" как событие чтения на fd. Можно добавить в epoll и ждать вместе с сокетами и сигналами.

```c
int tfd = timerfd_create(CLOCK_MONOTONIC, 0);  // fd типа timerfd
```

## Почему это важно

systemd-journald использует все три одновременно: epoll следит за сотней сокетов, signalfd ловит сигналы остановки, timerfd планирует периодический flush на диск. Всё через один `epoll_wait()`.

Это стандартный паттерн event-loop в системных демонах на Linux.

## Man pages / docs

- `man 7 epoll`
- `man 2 signalfd`
- `man 2 timerfd_create`

## Связанные темы

[[proc-filesystem]], [[symlinks-linux]]
