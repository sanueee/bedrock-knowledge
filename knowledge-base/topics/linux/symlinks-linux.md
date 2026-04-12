# Симлинки (symbolic links)

## Суть (своими словами)

Симлинк — файл который хранит внутри себя **строку-путь** до другого файла. Когда ядро видит обращение к симлинку — прозрачно перенаправляет на цель.

Это не копия файла и не жёсткая ссылка. Просто указатель. Цель может не существовать — симлинк при этом создать можно, он будет «битым».

Создать в шелле:
```bash
ln -s /home/user/real_file.txt mylink   # создать симлинк
ls -la mylink                           # mylink -> /home/user/real_file.txt
readlink mylink                         # /home/user/real_file.txt
```

## Зачем это в /proc

`/proc/<pid>/fd/` — директория, где каждый файл это симлинк:
- имя файла = номер fd (`0`, `1`, `4`, ...)
- цель симлинка = что реально открыто (`/dev/null`, `socket:[12345]`, ...)

Ядро создаёт эти симлинки автоматически при открытии ресурсов процессом. Это способ ядра **экспортировать** информацию об открытых fd наружу.

```
/proc/1234/fd/
  0  ->  /dev/null
  1  ->  /dev/null
  2  ->  /var/log/app.log
  4  ->  socket:[98765]
  5  ->  pipe:[11111]
```

## stat vs lstat

- `stat(path, &st)` — следует по симлинку, описывает **цель**
- `lstat(path, &st)` — **не** следует, описывает сам симлинк

```c
struct stat st;
lstat("/proc/1234/fd/4", &st);
if (S_ISLNK(st.st_mode)) {
    // это симлинк
}
```

## readlink

Читает строку-путь внутри симлинка. Не следует по ней.

```c
#include <unistd.h>

char target[256];
ssize_t len = readlink("/proc/1234/fd/4", target, sizeof(target) - 1);
if (len == -1) {
    perror("readlink");
} else {
    target[len] = '\0';  // readlink не добавляет \0 — нужно вручную
    printf("%s\n", target);  // /home/user/file.txt
}
```

## Пример из моего кода

```c
// из fdlist.c
char full_path[128];
snprintf(full_path, sizeof(full_path), "%s/%s", path, curr_fd->d_name);

char target[256];
ssize_t len = readlink(full_path, target, sizeof(target) - 1);
if (len == -1) continue;
target[len] = '\0';

fprintf(stdout, "fd %s -> %s\n", curr_fd->d_name, target);
```

## Man pages / docs

- `man 2 readlink`
- `man 2 lstat`
- `man 1 ln` — как создавать симлинки из шелла
- `man 7 symlink` — полное описание семантики

## Связанные темы

[[proc-filesystem]], [[file-io-c]], [[directory-traversal-c]]
