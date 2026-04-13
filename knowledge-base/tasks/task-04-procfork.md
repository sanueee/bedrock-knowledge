# Task 04 — procfork

## Задание

Утилита на C: принимает число N как аргумент, создаёт N дочерних процессов через `fork()`, собирает их через `waitpid()`.

```
$ ./procfork 3
[child 0] pid=1234 ppid=1233
[child 1] pid=1235 ppid=1233
[child 2] pid=1236 ppid=1233
[parent] child 1234 exited with code 0
[parent] child 1235 exited with code 1
[parent] child 1236 exited with code 2
parent done, collected 3 children
```

Требования:
- Создать N процессов через `fork()` в цикле
- Каждый дочерний: вывести свой pid/ppid, завершиться с кодом равным своему индексу (`exit(i)`)
- Родитель: `waitpid()` в цикле, вывести pid и exit code каждого дочернего
- Обработать ошибку `fork()` (возврат -1)

Файл: `linux/proc/procfork.c`

Функции: `fork()`, `waitpid()`, `getpid()`, `getppid()`, `WEXITSTATUS()`, `exit()`

## Моё решение

## Ключевые функции

## Тест на Linux

## Ошибки и трудности

## Что бы сделал иначе

## Связанные темы

[[proc-filesystem]]
