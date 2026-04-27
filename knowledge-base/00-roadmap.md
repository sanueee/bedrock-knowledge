---
обновлено: 2026-04-28
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

## Темы — C

- [[file-io-c]] — fopen/fgets/fclose vs open/read
- [[directory-traversal-c]] — opendir/readdir/closedir
- [[string-formatting-c]] — snprintf, sscanf, strncmp
- [[error-handling-c]] — обработка ошибок, errno, perror
- [[sorting-c]] — qsort, компаратор

---

## Стратегия

`.claude/strategy/learning-strategy.md`
