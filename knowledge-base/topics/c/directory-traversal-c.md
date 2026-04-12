# Обход директорий в C

## Суть (своими словами)

Для перебора содержимого директории в C используется POSIX API: `opendir` / `readdir` / `closedir`. Возвращает структуры `dirent` с именем файла и типом записи.

Нужно для task-02: перебрать все `/proc/<pid>/` директории.

## Пример — перебор /proc

```c
#include <dirent.h>
#include <stdlib.h>
#include <ctype.h>

DIR *d = opendir("/proc");
if (d == NULL) {
    perror("opendir");
    return;
}

struct dirent *entry;
while ((entry = readdir(d)) != NULL) {
    // entry->d_name — имя записи (строка)
    // entry->d_type — тип (DT_DIR, DT_REG, ...)

    // Фильтруем: имя — число => это PID
    // strtol читает число и ставит endptr на первый непрочитанный символ
    // если *endptr == '\0' — вся строка была числом
    char *endptr;
    strtoul(entry->d_name, &endptr, 10);
    if (*endptr == '\0' && endptr != entry->d_name) {
        // это PID-директория
    }
}

closedir(d);
```

## Структура dirent

```c
struct dirent {
    ino_t  d_ino;        // inode номер
    char   d_name[256];  // имя файла/директории
    // и другие поля...
};
```

## Важные детали

**`readdir` возвращает NULL двух случаях**: конец директории и ошибка. Чтобы различить — сбросить `errno` до цикла и проверить после.

**`d_type` не гарантирован**: на некоторых файловых системах `d_type` будет `DT_UNKNOWN`. Тогда нужен `stat`. Для `/proc` на Linux работает.

**`.` и `..`** — `readdir` всегда вернёт эти записи. Нужно фильтровать.

## Частая ошибка: забыть closedir

Аналогично `fclose`: утечка файловых дескрипторов.

## Man pages / docs

- `man 3 opendir`
- `man 3 readdir`
- `man 3 closedir`
- `man 5 dirent` — структура dirent

## Связанные темы

[[proc-filesystem]], [[file-io-c]]
