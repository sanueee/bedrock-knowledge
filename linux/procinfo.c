#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "error: argument required");
        return 1;
    }
    char *endptr;
    errno = 0;
    uint64_t pid = strtoull(argv[1], &endptr, 10);
    if (argv[1] == endptr)
    {
        fprintf(stderr, "error: int required");
        return 1;
    }
    if (errno == ERANGE)
    {
        fprintf(stderr, "error: int overflow");
        return 1;
    }

    char pid_string[21];
    snprintf(pid_string, sizeof(pid_string), "%llu", pid);

    char path[64] = "/proc/";
    strncat(path, pid_string, 21);
    strncat(path, "/status", 7);

    FILE* inp = fopen(path, "r");
    if (inp == NULL)
    {
        fprintf(stderr, "erorr: opening %s", path);
        return 1;
    }
    
    char line[128];
    while (fgets(line, sizeof(line), inp))
    {
        if (strncmp(line, "Name:", 5) == 0) {
            char name[64];
            if (sscanf(line + 5, "%63s", name) == 1) {
                fprintf(stdout, "Process:\t%s\n", name);
            }
        }
        else if (strncmp(line, "Pid:", 4) == 0) {
            char pid_val[21];
            if (sscanf(line + 4, "%20s", pid_val) == 1) {
                fprintf(stdout, "PID:\t%s\n", pid_val);
            }
        }
        else if (strncmp(line, "VmRSS:", 6) == 0) {
            char val[64], units[16];
            if (sscanf(line + 6, "%63s %15s", val, units) == 2) {
                uint64_t res_kb = strtoull(val, NULL, 10);
                uint64_t res_mb = res_kb / 1024;
                fprintf(stdout, "Memory:\t%llu MB\n", res_mb);
            }
        }
    }
    fclose(inp);
    return 0;
}

/*
Работает, ресурсы закрыты, конвертация есть. Задание выполнено — делаю код-ревью.

---

**1. Форматирование вывода**
Ты используешь `\t`, но в задании выравнивание пробелами:
```
Process: firefox
PID:     1234
Memory:  312 MB
```
Используй `%-8s` или просто захардкодь пробелы: `"Process: %s\n"`.

---

**2. Сборка пути — три вызова вместо одного**
```c
char path[64] = "/proc/";
strncat(path, pid_string, 21);
strncat(path, "/status", 7);
```
Это можно и нужно заменить на один `snprintf`:
```c
char path[64];
snprintf(path, sizeof(path), "/proc/%llu/status", pid);
```
Заодно избавишься от промежуточного `pid_string`.

---

**3. `%llu` для `uint64_t` — непортируемо**
Для `uint64_t` правильный спецификатор из `<inttypes.h>`:
```c
printf("%" PRIu64 "\n", pid);
```
На Linux с gcc `%llu` работает, но это не гарантия.

---

**4. Ошибки не сообщают `\n`**
```c
fprintf(stderr, "error: argument required");
```
Без `\n` в конце сообщение может слипнуться с приглашением терминала.

---

**5. Нет обработки: а вдруг поля нет в файле?**
Если процесс — поток ядра, `VmRSS` может отсутствовать. Программа молча завершится без вывода. Стоит проверять что все три поля найдены и сообщать об ошибке если нет.

---

Итог: логика правильная, структура чистая. 
Для первой утилиты на C — хорошо. Главное к запомнить: `snprintf` для форматирования строк вместо `strncat`-цепочек, и `\n` в stderr-сообщениях.
*/