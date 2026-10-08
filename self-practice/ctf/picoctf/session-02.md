---
session: 02
platform: picoctf
date: 2026-05-22
category: Forensics
difficulty: Easy
status: done
---

# CTF Session 02 — 2026-05-22

## Цель сессии
Forensics, Easy, 2–3 таска. Фокус: освоить базовые инструменты разбора артефактов — `file`, `strings`, `xxd`, `exiftool`, `sha256sum`, `dd`. Мост к B2 (libpcap).

## Решённые таски (4)

1. **CanYouSee** — base64 в `Attribution URL` (XMP-метаданные), `exiftool` + `base64 -d`.
2. **Verify** — `sha256sum files/* | grep <hash>` для поиска нужного файла среди многих, затем `./decrypt.sh` (OpenSSL `Salted__`).
3. **Corrupted file** — JPEG с подменёнными magic bytes (`5c 78` вместо `FF D8`). Починка `printf '\xff\xd8' | dd of=file conv=notrunc bs=1 count=2`.
4. **Secret of the Polyglot** — PNG+PDF в одном файле. `exiftool` пометил `Trailer data after IEND`; `grep -aob '%PDF'` дал offset, `dd skip=` извлёк PDF. Флаг из двух половинок (PNG + PDF).

## Нерешённые / частично
Нет — все 4 закрыты.

## Новые инструменты / приёмы

- `exiftool` — метаданные изображений (EXIF/XMP/ICC); подозрительное поле = указатель на флаг.
- `strings file | grep '==$'` — поиск base64 в бинаре без exiftool.
- `xxd file | head` — magic bytes; сравнить с эталонной таблицей.
- `file <f>` → `data` — сигнал что magic bytes сломаны или это polyglot.
- `sha256sum files/* | grep <hash>` — найти файл по хешу среди многих.
- `Salted__` префикс — фирменный маркер `openssl enc -salt`; не пытаться открывать редактором.
- `dd conv=notrunc bs=1 count=N` — точечная правка байтов **без** обрезания файла.
- `grep -aob '<magic>'` — найти offset вложенного формата в polyglot.
- `dd if=... of=... bs=1 skip=<offset>` — извлечь срез файла начиная с offset.

## Главный вывод сессии

Главный пробел — **утилитарная грамотность**. Многие подходы кажутся магией пока не знаешь утилиту и нужный флаг. После знакомства задача становится Easy. Magic bytes таблица + флаги `dd` (`conv=notrunc`, `seek`, `skip`) + `grep -aob` — must-know для forensics.

## Связанные темы
- [[session-02-writeup]] — сводный конспект по 4 таскам, [self-practice/ctf/picoctf/forensics/session-02-writeup.md](forensics/session-02-writeup.md).
- [[command-injection]] — session-01.
