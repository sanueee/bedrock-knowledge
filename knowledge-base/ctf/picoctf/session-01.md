---
session: 01
platform: picoctf
date: 2026-04-30
category: General Skills
difficulty: Easy
status: completed
---

# CTF Session 01 — 2026-04-30

## Цель сессии

Первая CTF-сессия на picoCTF. Категория **General Skills**, сложность **Easy**.

## Решённые таски

- **ping-cmd** (Easy, picoCTF 2026) — command injection через `&&` в форму ping. Writeup: [[ctf/picoctf/general-skills/ping-cmd]].

## Нерешённые / частично

—

## Новые инструменты / приёмы

- **`nc <host> <port>`** — netcat: подключение к TCP-сервису для интерактивного ввода. На macOS соединение закрывается после первой команды-ответа сервера — нормальное поведение.
- **Shell metacharacters как векторы инъекции**: `;`, `&&`, `||`, `|`, `` ` ``, `$(...)`. Запомнено отличие space (argument separator) vs `;`/`&&` (command separator).
- **Семантика `;` vs `&&` vs `||` vs `|`** — таблица в [[ctf/picoctf/general-skills/ping-cmd]].
- **Принцип `execve` over `system`** — для builder СЗИ запомнить навсегда: пользовательский ввод через shell = command injection.

## Связанные темы

- [[topics/security/command-injection]] — концепция (создана в этой сессии)
- Связь с [[topics/linux/pipes]] (`|` как pipe из task-a06)
- Связь с [[topics/linux/fork-exec-wait]] (`execve` vs `system` из task-a04)
