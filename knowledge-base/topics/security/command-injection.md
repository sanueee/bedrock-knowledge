---
topic: command-injection
category: security
date: 2026-04-30
source: ctf/picoctf/general-skills/ping-cmd
---

# Command Injection

## Суть (своими словами)

Уязвимость возникает когда программа склеивает пользовательский ввод в строку команды и передаёт её **через shell** (а shell интерпретирует metacharacters). Атакующий вставляет `;`, `&&`, `||`, `|`, `` ` ``, `$(...)` и выполняет произвольную команду.

Корень проблемы — **смешение слоёв**: данные (input) и код (shell command) попадают в один и тот же парсер.

## Минимальный уязвимый пример (C)

```c
char cmd[256];
snprintf(cmd, sizeof(cmd), "ping -c 2 %s", user_input);
system(cmd);   // ← shell парсит ВСЁ, включая ; && | $()
```

Если `user_input = "8.8.8.8 && cat /etc/passwd"` — выполнятся **две** команды.

## Безопасный аналог

```c
char *argv[] = {"ping", "-c", "2", user_input, NULL};
execve("/bin/ping", argv, envp);
// shell не запускается, user_input — это ОДИН аргумент,
// metacharacters трактуются как обычные символы
```

## Векторы внедрения (shell metacharacters)

| Символ | Эффект |
|--------|--------|
| `;` | Разделитель команд (обе выполняются) |
| `&&` | Условная последовательность (если предыдущая успешна) |
| `\|\|` | Условная последовательность (если предыдущая упала) |
| `\|` | Pipe — stdout → stdin |
| `` ` `` ... `` ` `` | Command substitution (старый синтаксис) |
| `$(...)` | Command substitution |
| `&` | Запуск в фоне |
| `\n` | Перевод строки = разделитель команд |
| `> < >>` | Redirection (запись/чтение в файл) |
| `*  ? [...]` | Globbing (раскрытие имён файлов) |

## Стратегии защиты (по убыванию надёжности)

1. **Не запускать shell вообще** — использовать `execve()` / `posix_spawn()` / `subprocess.run([list], shell=False)`. Аргументы передаются массивом, shell-парсинг отсутствует.
2. **Allowlist** (whitelist) — проверка что ввод содержит **только** разрешённые символы (например, `[0-9.]+` для IP).
3. **Escaping** — экранирование metacharacters перед передачей в shell. Хрупко, легко ошибиться (`$IFS`, многобайтные кодировки, локаль).
4. **Denylist** (blacklist) — фильтрация "опасных" символов. **Самый ненадёжный** способ: всегда найдётся обход (newline, null byte, unicode-вариант).

## Где встречается

- Web-приложения: backend склеивает параметр запроса в `os.system()` / `exec()` / `Runtime.exec()`.
- IoT / embedded: web-интерфейсы роутеров часто содержат command injection в diagnostic-формах (ping, traceroute, DNS lookup).
- DevOps tooling: shell-обёртки над `kubectl`, `terraform`, `git` с непроверенным вводом.
- CI/CD: build-скрипты которые подставляют git tag / branch name в команды.

## Связь с builder-вектором СЗИ

При написании СЗИ-агента на C/Rust:
- Никаких `system()`, `popen()`, `Runtime.exec("sh -c ...")`.
- Только `execve()`-семейство с массивом аргументов.
- Если разворачиваешь чужой sandbox / антивирус-движок и видишь `system()` — это red flag, чинить.

При chroot/namespace-изоляции (этап 3, блок G) — даже если атакующий получил injection, sandbox должен ограничить blast radius.

## Ключевые термины (English)

- **command injection** / **OS command injection** — class of vulnerability
- **shell metacharacter** — special characters parsed by shell
- **argument vector (argv)** — array of arguments passed to a process
- **shell expansion** — globbing, variable expansion, command substitution
- **input validation** — process of checking input against expected format
- **allowlist** / **denylist** — validation strategies (formerly whitelist/blacklist)
- **OWASP Top 10** — A03:2021 Injection
- **CWE-78** — Improper Neutralization of Special Elements used in an OS Command

## Источники

- CTF practice: [[ctf/picoctf/general-skills/ping-cmd]]
- OWASP: https://owasp.org/www-community/attacks/Command_Injection
- CWE-78: https://cwe.mitre.org/data/definitions/78.html
