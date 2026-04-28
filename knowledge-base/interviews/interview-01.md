# Interview 01 — 2026-04-28

## Темы
- /proc filesystem, opendir/readdir (A1)
- fork/waitpid, зомби-процессы (A4)
- sigaction vs signal (A5)
- pipe/IPC, EOF и закрытие концов (A6)
- /proc/net/tcp, little-endian парсинг (A7)
- pthreads, data race, мьютексы (A8)

## Уверенно знает
- Зомби-процесс — термин и сам факт
- Little-endian в /proc/net/tcp — понял и объяснил корректно
- Data race — суть уловил, направление верное

## Нужно повторить
- `opendir()` vs `readdir()`: перепутал кто возвращает NULL при ошибке (opendir) и кто при конце директории (readdir) → `topics/c/directory-traversal-c.md`
- Зомби-процесс: причина проблемы не в дескрипторах, а в занятой записи таблицы процессов / PID → `topics/linux/fork-linux.md`
- `sigaction()` vs `signal()`: не знал про автосброс обработчика после первого срабатывания и про `sa_mask` → `topics/linux/signals-linux.md`
- Pipe EOF: не знал что `read()` зависнет если родитель не закроет свой write-end → `topics/linux/pipes-linux.md`

## NotebookLM Prompt

---
Я изучаю системное программирование на C под Linux. Вот что я уже прошёл:
- /proc filesystem: opendir(), readdir(), fopen(), fgets(), парсинг /proc/<pid>/status
- directory traversal: opendir/readdir/closedir, обработка NULL от readdir
- file descriptors: readlink(), /proc/<pid>/fd/, символические ссылки
- string formatting: snprintf(), sscanf(), strncmp()
- sorting: qsort(), компаратор с const void *
- error handling: errno, perror()
- fork/exec/wait: fork(), execv(), waitpid(), WEXITSTATUS()
- signals: sigaction(), sigset_t, SIGINT, SIGTERM, pause(), sa_mask
- pipes/IPC: pipe(), dup2(), read(), write(), EOF при закрытии write-end
- /proc/net/tcp: парсинг hex-адресов и портов, little-endian представление IP
- pthreads: pthread_create(), pthread_join(), pthread_mutex_t, data race

На мок-собесе я показал уверенные знания по:
- Зомби-процесс: термин, факт существования
- Little-endian в /proc/net/tcp: правильно объяснил порядок байт
- Data race: суть уловил — одновременный доступ без синхронизации

Но у меня есть пробелы в следующих темах:

1. opendir() vs readdir(): думал что opendir() возвращает NULL при конце директории — на самом деле opendir() возвращает NULL только при ошибке открытия (путь не существует, нет прав). NULL при конце возвращает readdir().

2. Зомби-процесс: думал что проблема зомби — незакрытые дескрипторы. На самом деле дескрипторы закрываются при завершении процесса. Проблема зомби — занятая запись в таблице процессов и невозможность переиспользовать PID.

3. sigaction() vs signal(): не знал что signal() сбрасывает обработчик в SIG_DFL после первого срабатывания (на большинстве систем). sigaction() этого не делает. Также не знал про sa_mask — блокировку других сигналов во время выполнения обработчика.

4. Pipe EOF: не знал что read() зависнет навсегда если родитель не закрыл свой write-end pipe после fork(). EOF приходит только когда все write-концы закрыты.

Пожалуйста, сфокусируй объяснение на слабых местах. Для каждого пробела:
1. Объясни концепцию точно и без упрощений
2. Покажи минимальный пример кода
3. Задай мне один проверочный вопрос по этой теме
---
