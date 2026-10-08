---
блок: B0
тип: reading + threat model
статус: выполнено
начато: 2026-05-19
завершено: 2026-05-20
---

# task-b01 — OWASP Top 10 (web): теория

## Задание

**Цель:** знать на пальцах все 10 категорий актуального OWASP Top 10 для web (редакция 2021). Это база и для собеседований (DevSecOps/AppSec), и для блока B (что атакуют поверх HTTP/TLS). Не код — чтение + конспект + threat-modeling в голове.

**Источник (primary):** https://owasp.org/Top10/ — официальная страница, редакция 2021. Прочитать вводную + все 10 категорий (A01–A10).

**Что прочитать по каждой A0X:**
- Description (что это).
- Common Weakness Enumerations (CWE) — какие CWE сюда падают.
- How to Prevent — митигации.
- Example Attack Scenarios — 1-2 примера.

**Категории (для самоконтроля — после чтения должен сам перечислить):**
1. A01 — Broken Access Control (включает IDOR, path traversal, force browsing).
2. A02 — Cryptographic Failures (раньше "Sensitive Data Exposure").
3. A03 — Injection (SQLi, NoSQLi, OS command injection, LDAP; сюда же XSS в 2021).
4. A04 — Insecure Design.
5. A05 — Security Misconfiguration (default creds, verbose errors, открытые S3).
6. A06 — Vulnerable and Outdated Components.
7. A07 — Identification and Authentication Failures.
8. A08 — Software and Data Integrity Failures (включая insecure deserialization, supply chain).
9. A09 — Security Logging and Monitoring Failures.
10. A10 — Server-Side Request Forgery (SSRF).

**Минимальная глубина по топ-4 (SQLi, XSS, SSRF, IDOR)** — это must-have для собеседования:
- **SQL injection**: classic vs blind (boolean/time-based), как работает prepared statements, почему `' OR 1=1 --` пробивает наивный `WHERE`.
- **XSS**: reflected vs stored vs DOM-based, что делает CSP, почему `innerHTML` опаснее `textContent`.
- **SSRF**: зачем атакующему вызов от лица сервера (cloud metadata 169.254.169.254, внутренние сервисы), что блокирует SSRF (allowlist хостов, не denylist).
- **IDOR**: почему `/api/orders/42` без authz-проверки = доступ к чужим заказам, разница authentication vs authorization.

**Что создать в vault:** `knowledge-base/self-study/topics/security/owasp-top10.md`.
Структура (короткая):
```
# OWASP Top 10 (web, 2021)

## A01–A10 — однострочник на каждую
Список из 10 категорий, у каждой — 1-2 предложения "что это" + ключевой CWE.

## Глубокий разбор: SQLi, XSS, SSRF, IDOR
По 1 абзацу на атаку: как работает, мини-пример (одна строка кода или URL), как предотвращают.

## Ключевые термины (English)
Глоссарий: injection, sanitization, allowlist/denylist, prepared statement, parameterized query,
authn vs authz, IDOR, SSRF, CSP, same-origin policy, deserialization, supply chain attack.

## Связки с блоком B
Что из OWASP всплывёт когда дойдём до HTTP/TLS (B5).
```

**Практика (опционально, в следующую сессию или позже):** 3–5 челленджей на DVWA или PortSwigger Web Security Academy (бесплатные labs по SQLi/XSS). Не блокирует завершение B0 — основная цель сейчас теория.

## Перед началом

- Конспекта `topics/security/owasp-top10.md` ещё нет — создаём с нуля через `vault-write` по итогам сессии.
- Связанный конспект уже есть: [[command-injection]] (топик из CTF, ping-cmd) — это частный случай A03 Injection. При записи owasp-top10.md можно сослаться через `[[command-injection]]`.

## Моё решение

Прочитал OWASP Top 10 2021 (edition 2025 ещё в драфте) — после чтения было "очень много мест где потенциально может быть уязвимость", поверхностный обзорный уровень. Чтобы каркас встал, прошёл мини-самопроверку: 5 вопросов на authn vs authz, SQLi/prepared statement, XSS типы, SSRF, IDOR. По итогам стало видно 4 пробела:
- prepared statement: знал что "нужно", не знал **механизма** (двухэтапная отправка — prepare план без значений, execute с типизированными параметрами; данные не попадают в парсер SQL).
- XSS: знал только stored, не различал reflected/DOM-based.
- SSRF: нулевое понимание — закрылось через cloud metadata `169.254.169.254` и кейс Capital One.
- IDOR: путал "доступность URL" с "проверкой прав". Различие — откуда сервер берёт ID объекта (клиент vs сессия).

После этого пошёл углублённый разбор по всем 10 категориям (механизм атаки → почему защита именно такая) и принципы которые важнее самих категорий (defense in depth, структурные защиты > текстовые, не доверять клиенту). Конспект записан в [[owasp-top10]].

DVWA/PortSwigger labs пока не делал — отложено, не блокирует завершение B0.

## Ключевые концепции

| Концепция | Механизм |
|-----------|----------|
| authn vs authz | login прошёл ≠ можно делать действие; A07 — сломан вход, A01 — вход прошёл, дальше не проверили |
| prepared statement | prepare парсит SQL без значений → execute с типизированными параметрами; данные вне грамматики SQL |
| XSS reflected/stored/DOM | где живёт скрипт: URL-параметр / БД / клиентский JS из location.hash |
| SSRF + cloud metadata | сервер ходит изнутри сети; `169.254.169.254` отдаёт IAM-credentials без auth |
| IDOR | ID из клиентского запроса без authz-проверки |
| DNS rebinding | обход IP-denylist через смену DNS-ответа между валидацией и запросом |
| allowlist vs denylist | allowlist выживает обходы, denylist — нет |
| password hashing | bcrypt/argon2id (slow + salt + memory-hard), не SHA-256 |
| supply chain attack | компрометация зависимости (xz-utils 2024, event-stream, SolarWinds); защита — SBOM + signing + SLSA |

## Что узнал

- OWASP — не список атак, а **10 категорий мышления**. Один паттерн ("данные стали кодом" → A03) даёт целый класс атак: SQLi, command injection, XSS, LDAP injection, NoSQL injection.
- **Структурные защиты сильнее текстовых.** Prepared statement не "очищает", а **выносит данные из грамматики** — атакующий не может повлиять на план выполнения. Аналогично: `execve(argv[])` vs `system(string)`, `textContent` vs sanitize-HTML, allowlist vs denylist.
- **Insecure Design (A04) — отдельная категория.** Код корректный, дизайн дырявый. SAST/DAST его не ловят — нужен threat modeling на стадии proposal.
- Связка с моей траекторией: A06/A08 — это **DevSecOps-стек** (SCA, SBOM, supply chain, SLSA). A09 — то что покрывает eBPF-телеметрия из этапа 3 на уровне ядра, где app-логи слепые.

## Ошибки и трудности

- На вопрос дебрифа "что показалось самым новым" — не смог выделить одно: "тяжело выделить что то одно — надо будет погружаться во все". Это честное отражение поверхностного уровня после первого прохода. Каркас встал, но ни одна категория ещё не "своя".
- Изначально на самопроверке IDOR показалось "это же просто URL" — не разделял "URL доступен" и "сервер проверил права на этот объект". Сейчас понятно, но не факт что устойчиво — нужна практика.

## Что бы сделал иначе

- Сразу после чтения OWASP-страницы — самопроверку на каждый из топ-4 (SQLi, XSS, SSRF, IDOR), не ждать что "усвоилось". Поверхностное чтение даёт **карту**, не **знание**. Самопроверка форсирует обнаружение пробелов.
- DVWA/PortSwigger labs — добавить отдельной сессией перед B5 (TLS). Тогда теория зафиксируется через руки.

## Открытые вопросы — куда вернуться

1. **CSP** — какие директивы, как nonce/hash работают, как настраивать на реальном сайте.
2. **DNS rebinding** — точная механика TTL, как браузер/HTTP-клиент его учитывает.
3. **Insecure deserialization gadget chains** — как именно RCE собирается из "невинных" классов (на примере Java/Python).
4. **JWT pitfalls** — `alg: none`, key confusion, отсутствие revocation.
5. **SLSA framework** — уровни (L1-L4), что подписывают, как verify в CI.
6. **Practice gap** — все 10 категорий пока теория. DVWA/PortSwigger Web Security Academy (бесплатные labs) — добавить отдельной сессией.

## Связанные темы

[[owasp-top10]] [[command-injection]] [[tcp-sockets]]
