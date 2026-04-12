# /proc — псевдофайловая система

## Суть (своими словами)

`/proc` — это не настоящая файловая система на диске. Ядро Linux генерирует её содержимое на лету при каждом обращении. Каждый запущенный процесс имеет директорию `/proc/<pid>/` с информацией о себе.

Для нас как инструментальщиков — это API ядра в виде текстовых файлов.

## Структура `/proc/<pid>/`

| Файл | Что содержит |
|---|---|
| `status` | Имя, PID, состояние, память (VmRSS, VmSize и др.) |
| `cmdline` | Аргументы с которыми запущен процесс (разделены `\0`) |
| `maps` | Карта памяти: какие регионы куда отображены |
| `fd/` | Директория: симлинки на открытые файловые дескрипторы |
| `stat` | Числовые данные (использует ps, top) |

## Пример из моего кода

```c
// Читаем VmRSS из /proc/<pid>/status
char path[64];
snprintf(path, sizeof(path), "/proc/%llu/status", pid);

FILE* inp = fopen(path, "r");
char line[128];
while (fgets(line, sizeof(line), inp)) {
    if (strncmp(line, "VmRSS:", 6) == 0) {
        // VmRSS:   312 kB
        uint64_t res_kb;
        sscanf(line + 6, "%" SCNu64, &res_kb);
    }
}
```

## Важные поля status

```
Name:   firefox          <- имя процесса (max 15 символов)
Pid:    1234             <- PID
PPid:   1000             <- родительский PID
VmRSS:  312040 kB        <- реальная RAM сейчас
VmSize: 524288 kB        <- виртуальная память
Threads: 8               <- количество потоков
```

`VmRSS` (Resident Set Size) — сколько физической памяти сейчас занято. Именно это показывает `top`.

## Перебор всех процессов

```c
// /proc/ содержит директории с числовыми именами — это PID-ы
DIR *d = opendir("/proc");
struct dirent *entry;
while ((entry = readdir(d)) != NULL) {
    // entry->d_name — имя записи
    // Если это число — это PID
}
closedir(d);
```

## Man pages / docs

- `man 5 proc` — полная документация /proc
- `man 2 open` — low-level open
- `man 3 fopen` — buffered fopen

## /proc/<pid>/fd/

Директория симлинков на открытые файловые дескрипторы процесса.

```bash
ls -la /proc/1234/fd/
# 0 -> /dev/null
# 1 -> /dev/pts/0
# 4 -> socket:[98765]
```

Перебирается через `opendir`/`readdir`, каждая запись читается через `readlink`.

## Связанные темы

[[file-io-c]], [[directory-traversal-c]], [[symlinks-linux]]
