---
тема: Файловый ввод-вывод в C / File I/O
блок: A — Linux syscalls
дата: 2026-04-13
связано:
  - "[[error-handling-c]]"
  - "[[proc-filesystem]]"
---

# Файловый ввод-вывод в C

## Суть (своими словами)

В C есть два уровня работы с файлами:
- **Низкоуровневый** (`open/read/write/close`) — системные вызовы, без буфера
- **Буферизованный** (`fopen/fgets/fprintf/fclose`) — стандартная библиотека поверх syscall-ов

## Когда что использовать

| Ситуация | Выбор | Почему |
|---|---|---|
| Читать текстовый файл построчно | `fopen` + `fgets` | `fgets` — готовая функция, буфер от библиотеки |
| Бинарный файл, точный контроль | `open` + `read` | Нет скрытой буферизации |
| Нужны флаги типа `O_NONBLOCK`, `O_CREAT` | `open` | У `fopen` нет таких флагов |
| Работа с /proc текстовыми файлами | `fopen` | Удобнее |

## Пример из моего кода

```c
// Открыть и построчно читать /proc/<pid>/status
FILE* inp = fopen(path, "r");
if (inp == NULL) {
    fprintf(stderr, "error: opening %s\n", path);
    return;
}

char line[128];
while (fgets(line, sizeof(line), inp)) {
    // line содержит одну строку, включая \n
    if (strncmp(line, "Name:", 5) == 0) {
        // обработка
    }
}
fclose(inp);  // всегда закрывать!
```

## Частые ошибки

**Забыть закрыть файл** — утечка файловых дескрипторов. В системе их ограниченное количество (обычно 1024 на процесс).

**Не проверить возврат fopen** — если файл не существует или нет прав, `fopen` вернёт `NULL`. Разыменование NULL — crash.

**Буфер `fgets` включает `\n`** — строка "Name:\tfirefox\n" — `\n` входит в буфер. При сравнении это важно.

## fopen vs open — конкретный пример

```c
// fopen — высокоуровневый
FILE *f = fopen("/proc/1/status", "r");
fgets(buf, sizeof(buf), f);  // удобно

// open — низкоуровневый
int fd = open("/proc/1/status", O_RDONLY);
read(fd, buf, sizeof(buf));  // нет встроенной разбивки на строки
close(fd);
```

## Man pages / docs

- `man 3 fopen` — открыть файл (буферизованно)
- `man 3 fgets` — читать строку
- `man 2 open` — открыть файл (syscall)
- `man 2 read` — читать байты (syscall)

## Связанные темы

[[proc-filesystem]], [[error-handling-c]]
