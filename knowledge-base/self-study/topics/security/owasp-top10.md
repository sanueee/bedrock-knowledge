---
тема: OWASP Top 10 (web, 2021) / OWASP Top 10 Web Application Security Risks
блок: Security/CTF
дата: 2026-05-20
связано:
  - "[[command-injection]]"
  - "[[tcp-sockets]]"
---

# OWASP Top 10 (web, 2021) / OWASP Top 10 Web Application Security Risks

## Что это

Список 10 категорий наиболее критичных рисков для web-приложений, поддерживается OWASP (Open Worldwide Application Security Project). Не "10 атак", а **10 категорий мышления** — внутри каждой десятки реальных техник. Редакция 2021 актуальна на 2026-05; редакция 2025 в драфте.

## Ключевые термины (English)

- **authn (authentication)** — проверка "кто ты"
- **authz (authorization)** — проверка "тебе можно это действие"
- **IDOR** (Insecure Direct Object Reference) — частный случай Broken Access Control
- **RBAC / ABAC** — role-based / attribute-based access control
- **prepared statement** / **parameterized query** — SQL с плейсхолдерами, структурная защита от SQLi
- **escaping / sanitization** — текстовая защита (модифицируем строку), слабее prepared statement
- **CSP** (Content Security Policy) — HTTP-заголовок, ограничивает что браузер может выполнить
- **same-origin policy / SOP** — браузерное правило изоляции между origin
- **HSTS** (HTTP Strict Transport Security) — заголовок "только HTTPS"
- **password hashing function** — bcrypt / argon2id / scrypt (slow + salt)
- **CVE** (Common Vulnerabilities and Exposures) — публичный идентификатор уязвимости
- **SBOM** (Software Bill of Materials) — манифест зависимостей артефакта (SPDX, CycloneDX)
- **SCA** (Software Composition Analysis) — сканеры зависимостей (Dependabot, Trivy, Snyk)
- **MFA** (Multi-Factor Authentication) — TOTP, WebAuthn, hardware key
- **credential stuffing** — атака утёкшим логин/пароль с другого сайта
- **session fixation** — атакующий подбрасывает session ID жертве
- **insecure deserialization** — десериализация недоверенного объекта → RCE
- **supply chain attack** — компрометация зависимости (event-stream, xz-utils, SolarWinds)
- **SSRF** (Server-Side Request Forgery) — заставить сервер сходить по URL за себя
- **cloud metadata service** — `169.254.169.254`, link-local IP, отдаёт IAM-credentials изнутри VM
- **DNS rebinding** — обход IP-denylist через смену DNS-ответа между валидацией и запросом
- **forced browsing** — доступ к URL который не залинкован в UI, но и не защищён
- **threat modeling** — STRIDE и пр., анализ угроз на стадии дизайна
- **defense in depth** — несколько независимых слоёв защиты

## Категории — карта (A01–A10)

| # | Категория | Однострочник |
|---|-----------|--------------|
| A01 | Broken Access Control | сервер не проверяет права на конкретный объект/действие после login |
| A02 | Cryptographic Failures | sensitive data не шифруется или шифруется криво (MD5, ECB, hard-coded keys, plaintext в логах) |
| A03 | Injection | данные клиента попали в парсер кода (SQL, shell, LDAP, **HTML/JS = XSS**, NoSQL) |
| A04 | Insecure Design | дыра в архитектуре, не в коде (rate-limit отсутствует, цена в hidden field, права на клиенте) |
| A05 | Security Misconfiguration | default creds, verbose errors, открытые порты, debug в проде, открытые S3 |
| A06 | Vulnerable and Outdated Components | старая библиотека с известной CVE (Equifax/Struts, Log4Shell) |
| A07 | Identification and Authentication Failures | weak passwords, отсутствие MFA, brute force без rate-limit, session fixation |
| A08 | Software and Data Integrity Failures | insecure deserialization (pickle/Java), supply chain (xz-utils, event-stream), CI/CD без подписи |
| A09 | Security Logging and Monitoring Failures | атаку никто не заметил; либо в логах sensitive data |
| A10 | Server-Side Request Forgery | сервер ходит по URL от пользователя → достаёт внутренние сервисы / cloud metadata |

## Глубокий разбор четырёх ключевых

### SQL injection и prepared statement (A03)

Уязвимо: `SELECT * FROM users WHERE name = 'INPUT'`. INPUT = `' OR '1'='1` → всегда true.

**Prepared statement** — структурная защита. Два этапа:
1. **Prepare**: SQL с плейсхолдерами `WHERE name = ?` отправляется в DB. Движок парсит и строит execution plan **без значений**.
2. **Execute**: значения шлются отдельным сообщением как **типизированные параметры**, не вставляются в SQL-текст. Парсер уже отработал — повлиять на план невозможно.

`' OR '1'='1` остаётся строкой, потому что DB не парсит его как SQL. Это **не** "очистили опасные символы", это "данные физически вне грамматики SQL".

Escaping — другой подход (модифицируем текст). Работает, но один пропуск = дыра. Prepared statement предпочтительнее.

### XSS — три типа (A03)

Браузер парсит HTML; встретив `<script>`, выполняет JS в контексте сайта (с cookies жертвы).

- **Reflected** — скрипт в URL-параметре, сервер вставляет в ответ без эскейпинга. Требует social engineering (жертва кликает ссылку).
- **Stored** — скрипт сохранён на сервере (комментарий, профиль). Жертвой становится каждый, кто открывает страницу.
- **DOM-based** — сервер не участвует. Клиентский JS читает из `location.hash`/`document.URL` и пишет в `innerHTML`. WAF на сервере не видит.

Защита: output encoding **в контексте** (HTML/JS/URL — разный), **CSP** как defense in depth, безопасные API (`textContent` вместо `innerHTML`).

### SSRF и cloud metadata (A10)

Атакующий снаружи **не может** достать внутренние IP жертвы. Если у сервера есть фича "сходи по URL" — атакующий говорит серверу сходить. Сервер ходит **изнутри сети**.

Главная цель — **cloud metadata service** `169.254.169.254` (AWS/GCP/Azure). Link-local IP, доступен только из VM, без auth. Отдаёт IAM-credentials текущей машины → атакующий получает облачные ключи. **Capital One breach 2019** — ровно этот сценарий, ~100 млн клиентов.

Обходы denylist: **DNS rebinding** (валидация на публичный IP, запрос на 169.254...), redirect через `attacker.com`, IPv6, десятичные IP. Поэтому защита — **allowlist** (явный список разрешённых хостов), **network segmentation** (запрет на link-local из приложения), **IMDSv2** в AWS (требует токен, не уязвим к SSRF без header injection).

### IDOR (A01)

`/api/orders/42` vs `/api/orders/my`:
- В первом случае ID приходит **от клиента** → сервер обязан проверить authz `if order.owner_id != session.user_id: 403`. Если забыли — Алиса меняет 42 на 43 и читает заказ Боба.
- Во втором ID берётся **из сессии** на сервере → нет поля куда подставить чужой ID.

Суть IDOR: сервер доверяет идентификатору из клиентского запроса без проверки прав. Частный случай Broken Access Control.

## Принципы которые важнее категорий

1. **Большинство дыр — отсутствие проверки.** Authz, валидация input, escaping output.
2. **Defense in depth.** CSP не заменяет escaping. Allowlist не заменяет network segmentation. MFA не заменяет password storage.
3. **Структурные защиты > текстовые.** Prepared statement > escaping. `execve` (массив argv) > `system` (shell-парсинг — см. [[command-injection]]). `textContent` > sanitize HTML.
4. **Никогда не доверять клиенту.** ID, роль, цена, флаги — не источник истины. Источник — сервер.
5. **OWASP — категории мышления.** SQLi, command injection, XSS — все в A03 потому что общий паттерн "данные стали кодом". Видишь паттерн — ловишь класс атак.

## Связь с блоком B и будущими треками

- **B5 TLS** — A02 Cryptographic Failures (TLS handshake, HSTS, ciphersuites, downgrade attacks).
- **DevSecOps stack** — A06 (SCA/SBOM/Dependabot/Trivy), A05 (kube-bench, IaC scanners), A08 (supply chain, SLSA).
- **Этап 3 LLM-jail** — eBPF-телеметрия покрывает A09 на уровне ядра (process exec, network connect — то, что app-логи не видят).
- **OWASP Top 10 for LLM** (блок Z1/I1) — параллельный список для LLM-приложений (prompt injection, insecure plugin design).

## Подводные камни — что легко перепутать

- **authn vs authz** — login прошёл (authn) не значит "можно делать это" (authz). Забыли проверить — A01.
- **Escaping vs prepared statement** — это разные уровни, не синонимы. Escaping чинит текст, prepared statement выносит данные из грамматики.
- **A01 vs A07** — A07 это "сломан вход", A01 это "вход прошёл, но дальше не проверили".
- **Denylist в SSRF не работает.** Только allowlist. DNS rebinding пробивает любой denylist.
- **SHA-256 для паролей — плохо.** Слишком быстрый. Только argon2id/bcrypt/scrypt.
- **Self-XSS не уязвимость.** Если юзер вставляет скрипт в свой devtools — это не дыра. Дыра — когда вектор доставки независим от жертвы.

## Связанные темы

[[command-injection]] (частный случай A03), [[tcp-sockets]] (TLS — A02), `[[ssrf]]` (будет), `[[prepared-statement]]` (будет), `[[csp]]` (будет)
