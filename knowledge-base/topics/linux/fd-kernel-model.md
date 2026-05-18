---
тема: Файловые дескрипторы в ядре / File descriptors kernel model
блок: Фундамент (theory)
дата: 2026-05-14
связано:
  - "[[fork-linux]]"
  - "[[pipes-linux]]"
  - "[[symlinks-linux]]"
  - "[[virtual-memory-cow]]"
  - "[[syscalls-linux]]"
---

# Файловые дескрипторы в ядре / File descriptors kernel model

## Что это

`fd` — это **не указатель на файл и не "номер файла"**. Это индекс в per-process таблице, которая указывает на ядерный объект (`struct file`) с **счётчиком ссылок** (reference count). Понимание трёх уровней критично для всего что связано с pipe/fork/dup/close.

## Ключевые термины (English)

- **fd table** — per-process массив указателей на `struct file`, индексируется fd-числом
- **struct file** — open file description, ядерный объект описывающий открытый ресурс
- **f_count** — атомный счётчик ссылок на `struct file`
- **inode** — описание физического файла на диске (или pipe-buffer для пайпа)
- **refcount (reference count)** — счётчик сколько мест держат ресурс; 0 → ресурс освобождён
- **CLOEXEC** — флаг fd, заставляющий ядро закрыть fd при `execve`

## Три уровня

```
fd table (per-process)    →    struct file              →    inode / pipe-buffer
  [0] → struct file *A         A { f_count=2, f_pos,         (физический ресурс,
  [1] → struct file *A             f_flags, f_op, ... }       один на всех)
  [2] → struct file *B
  [3] → struct file *A
```

- **fd** — целое число, индекс в `current->files->fdt[]`.
- **`struct file`** — живёт в ядре, у него `f_count` (атомный), `f_pos` (текущая позиция), `f_flags` (O_RDONLY и т.д.), `f_op` (таблица операций — read/write/release).
- **inode / pipe-buffer** — физический ресурс. Один файл на диске, открытый дважды разными процессами, даст **два разных `struct file`** (со своими `f_pos`), но **один inode**.

## Что меняет f_count

| Операция | Эффект |
|----------|--------|
| `open(path, ...)` | Новый `struct file`, `f_count = 1` |
| `dup(fd)`, `dup2(fd, fd2)`, `fcntl(F_DUPFD)` | **Тот же** `struct file`, новый fd → `f_count++` |
| `fork()` | child копирует fd table → для **каждого** открытого fd `f_count++` |
| `close(fd)` | `f_count--`. Если стал 0 → ядро вызывает `f_op->release()` |
| `exit(2)` | Все fd процесса закрываются → `f_count--` для каждого |
| `execve(...)` | Закрываются только fd с флагом `O_CLOEXEC` |

`dup` и `fork` дают **одну и ту же `struct file`** — поэтому `f_pos` (позиция чтения) у родителя и ребёнка после fork **общая**. Это часто удивляет.

## EOF на pipe — это правило ядра

`read()` на read-end пайпа возвращает 0 (EOF) **тогда и только тогда, когда `f_count` на write-end = 0** — то есть **никто** не держит write-end. Не "когда родитель закрыл", а когда вообще все.

Это центральный механизм reading-01:

**Сетап**: P держит `wfd1` (write-end к C1). P делает `fork` → создаёт C2. Без защиты C2 наследует `wfd1` через fork-копирование fd table → `f_count` на write-end `wfd1` стал 2.

Дальше P делает `close(wfd1)`:
- `f_count: 2 → 1`. **EOF не приходит.**
- C1 виснет в `read()` ожидая EOF, который никогда не придёт.
- musl решает это явным `posix_spawn_file_actions_addclose` для каждого pipe-fd в ofl — чтобы C2 закрыл `wfd1` до `execve`. См. [[reading-01]] вывод 1.

## Пример

```c
int fd1 = open("/tmp/x", O_RDONLY);   // struct file A, f_count=1
int fd2 = dup(fd1);                    // тот же struct file A, f_count=2
pid_t pid = fork();                    // child: своя fd table, но struct file A, f_count=4
                                       // (2 в родителе + 2 в ребёнке)
close(fd1);                            // f_count: 4 → 3
// ...
```

## Подводные камни

- **`close(fd)` не всегда освобождает ресурс** — только декрементит. Если другой fd / другой процесс держит — `struct file` живёт.
- **CLOEXEC ставится на fd, не на `struct file`** — поэтому `dup` без `F_DUPFD_CLOEXEC` потеряет флаг, хотя `struct file` тот же.
- **fd-числа переиспользуются**: после `close(5)` следующий `open` может вернуть 5. После этого старые ссылки на "fd 5" указывают на новый объект — классическая ошибка race condition.
- **fd 0/1/2 (stdin/stdout/stderr)** — не магия, просто соглашение. Можно `close(1)` и потом `open(...)` вернёт 1.
- **На уровне ядра `struct file` живёт в slab-аллокаторе**, его адрес не виден из user space, но `lsof` показывает связь fd → inode через `/proc/PID/fd/`.

## Связанные темы

[[pipes-linux]] [[fork-linux]] [[symlinks-linux]] [[reading-01]] [[virtual-memory-cow]]
