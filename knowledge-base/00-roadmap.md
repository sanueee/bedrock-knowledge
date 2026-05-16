---
обновлено: 2026-05-17
---

# Roadmap

Хаб проекта. Все задачи, темы, прогресс — здесь.
Код: `linux/` | Теория: `topics/` | Задания: `tasks/`

---

## Задачи

| # | Задание | Статус | Код |
|---|---------|--------|-----|
| 01 | [[task-01-procinfo\|procinfo]] | ✅ выполнено | `procinfo.c` |
| 02 | [[task-02-procinfo-top5\|procinfo top5]] | ✅ выполнено | `procinfo2.c` |
| 03 | [[task-03-fdlist\|fdlist]] | ✅ выполнено | `fdlist.c` |
| 04 | [[task-04-procfork\|procfork]] | ✅ выполнено | `procfork.c` |
| 05 | [[task-05-signals\|signals]] | ✅ выполнено | `signals.c` |
| 06 | [[task-06-pipes\|pipes]] | ✅ выполнено | `pipechat.c` |
| 07 | [[task-07-procnet\|procnet]] | ✅ выполнено | `procnet.c` |
| 08 | [[task-08-pthreads\|pthreads]] | ✅ выполнено | `procthreads.c` |
| 09 | [[task-09-interview01-reinforce\|interview-01 reinforce]] | ✅ выполнено | `linux/reinforce/` |
| reading-01 | [[musl-popen\|musl popen/pclose/_Fork]] | ✅ выполнено (2026-05-08) | `topics/reading/` |
| theory | фундамент перед B1 (kernel/user, fd model, async-signal-safe, COW) | ✅ выполнено (2026-05-14) | `topics/linux/` |
| 10 | [[task-10-tcp-echo\|TCP echo server + client]] | ✅ выполнено (2026-05-17) | `networking/echo_server.c`, `networking/echo_client.c` |

---

## Темы — Linux

- [[proc-filesystem]] — файловая система /proc
- [[fork-linux]] — создание процессов, fork/waitpid
- [[signals-linux]] — сигналы, sigaction, SIGINT/SIGTERM
- [[pipes-linux]] — межпроцессное взаимодействие через pipe
- [[proc-net]] — чтение /proc/net/tcp, парсинг TCP-соединений
- [[pthreads-linux]] — потоки, pthread_create/join, мьютексы
- [[symlinks-linux]] — символические ссылки, readlink
- [[anon-inode]] — анонимные inode: epoll, signalfd, timerfd
- [[ssh-basics]] — основы SSH
- [[syscalls-linux]] — системные вызовы, kernel/user boundary
- [[fd-kernel-model]] — file descriptor table, struct file, refcount
- [[async-signal-safe]] — какие функции safe в обработчиках сигналов
- [[virtual-memory-cow]] — виртуальная память и copy-on-write при fork

## Темы — Networking

- [[tcp-sockets]] — TCP-сокеты, socket/bind/listen/accept/connect, partial read, EPIPE, SIGPIPE защита

## Темы — C

- [[file-io-c]] — fopen/fgets/fclose vs open/read
- [[directory-traversal-c]] — opendir/readdir/closedir
- [[string-formatting-c]] — snprintf, sscanf, strncmp
- [[error-handling-c]] — обработка ошибок, errno, perror
- [[sorting-c]] — qsort, компаратор

---

## Мок-собесы

| # | Дата | Слабые места |
|---|------|-------------|
| 01 | 2026-04-28 | opendir vs readdir, зомби/таблица процессов, sigaction vs signal, pipe EOF |

---

## CTF

| # | Дата | Платформа | Категория | Таск | Главное |
|---|------|-----------|-----------|------|---------|
| 01 | 2026-04-30 | picoCTF | General Skills | [[ctf/picoctf/general-skills/ping-cmd\|ping-cmd]] | command injection через `&&`; space vs shell metacharacter; `execve` over `system` |

---

## Стратегия

`.claude/strategy/learning-strategy.md`
