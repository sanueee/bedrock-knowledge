# Форматирование строк в C

## Суть (своими словами)

В C нет конкатенации строк через `+`. Для сборки строк — `snprintf`. Для вывода — `printf`/`fprintf`. Главное правило: всегда указывать максимальный размер буфера, иначе переполнение.

## snprintf — правильный способ собирать строки

```c
// Плохо — три вызова, лишняя переменная:
char pid_string[21];
snprintf(pid_string, sizeof(pid_string), "%llu", pid);
char path[64] = "/proc/";
strncat(path, pid_string, 21);
strncat(path, "/status", 7);

// Хорошо — один вызов:
char path[64];
snprintf(path, sizeof(path), "/proc/%" PRIu64 "/status", pid);
```

`snprintf` всегда добавляет `\0` и не пишет больше `n-1` символов. Безопасно.

## Спецификаторы для целых чисел

| Тип | Спецификатор | Заголовок |
|---|---|---|
| `int` | `%d` | — |
| `unsigned` | `%u` | — |
| `long long` | `%lld` | — |
| `unsigned long long` | `%llu` | — |
| `uint64_t` | `PRIu64` | `<inttypes.h>` |
| `int64_t` | `PRId64` | `<inttypes.h>` |

`%llu` для `uint64_t` работает на Linux x86-64, но **непортируемо**. Правильно:

```c
#include <inttypes.h>

uint64_t pid = 1234;
printf("PID: %" PRIu64 "\n", pid);  // правильно
printf("PID: %llu\n", pid);         // работает на Linux, но не гарантия
```

## Пример из моего кода

```c
// Извлечение значения из строки "VmRSS:   312040 kB"
char val[64], units[16];
if (sscanf(line + 6, "%63s %15s", val, units) == 2) {
    uint64_t res_kb = strtoull(val, NULL, 10);
    uint64_t res_mb = res_kb / 1024;
    fprintf(stdout, "Memory:\t%" PRIu64 " MB\n", res_mb);
}
```

## Форматирование вывода с выравниванием

```c
// Левое выравнивание на 10 символов:
printf("%-10s %s\n", "Process:", name);

// Результат:
// Process:   firefox
// PID:       1234
// Memory:    312 MB
```

## Man pages / docs

- `man 3 snprintf` — форматирование в буфер
- `man 3 printf` — форматированный вывод
- `man 3 sscanf` — разбор строки по формату

## Связанные темы

[[error-handling-c]], [[file-io-c]]
