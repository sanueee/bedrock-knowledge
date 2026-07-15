---
type: atomic
block-position: B-14
phase: 3
title: TLS handshake observational — Wireshark
status: выполнено
date: 2026-07-14
tags: [networking, tls, handshake, clienthello, serverhello, sni, alpn, wireshark]
---

# B-14 — TLS handshake observational

**Тип:** атомарное (позиция 14/26, Фаза 3 — application protocols + service detection). **Кода нет** — наблюдение и конспект.
**Синтез опоры:** TCP 3-way (весь блок B, `tcp-sockets.md`), протоколы поверх TCP (`http.md`, `dns.md`), классификация сервиса по содержимому ответа (`service-fingerprint.md`). TLS — это то, что происходит **между** `connect()` и первым HTTP-байтом на 443.
**Готовит:** понимание, почему на 443 нельзя просто `recv()` баннер (B-13 работал на plaintext-портах); задел под service detection в scanner v2 (B-15) и общий чекпойнт этапа 1 «понимаю, что происходит при `connect()`» — расширяем с L4 на L7.

Перечитать перед стартом: конспекта `topics/networking/tls.md` **ещё нет** — создадим после сессии. Опора: `topics/networking/service-fingerprint.md` (как читается ответ сервиса), `topics/networking/tcp-sockets.md` (что уже установлено к моменту старта TLS).

---

## Задание

Захватить **живой HTTPS-handshake** в Wireshark, расшифровать его (TLS 1.3 иначе не покажет ничего дальше `ServerHello`) и по байтам разобрать, что стороны сказали друг другу до первого HTTP-запроса. Итог — не файл, а **умение читать handshake** и ответы на контрольные вопросы ниже. Конспект в `tls.md` пишем после.

Разбор ведём на **одном** реальном хосте (предлагаю `example.com` или `cloudflare.com` — оба TLS 1.3, ALPN `h2`).

Ожидаемая картина в Wireshark после decrypt (TLS 1.3):

```
No.  Protocol  Info
1    TCP       [SYN] ...            ← это ты уже знаешь (блок B)
2    TCP       [SYN, ACK]
3    TCP       [ACK]
4    TLSv1.3   Client Hello         ← cleartext
5    TLSv1.3   Server Hello, Change Cipher Spec   ← ServerHello cleartext, дальше шифр
6    TLSv1.3   Application Data ...  ← БЕЗ decrypt: Encrypted Extensions / Certificate /
                                       CertificateVerify / Finished спрятаны здесь
     (после decrypt строка 6 разворачивается в отдельные Handshake-сообщения)
```

### Центральная идея — handshake решает три задачи разом

TLS-рукопожатие до единого байта прикладных данных должно:
1. **Договориться о параметрах** (версия TLS, cipher suite, группа ECDHE) — negotiation.
2. **Аутентифицировать сервер** (Certificate + CertificateVerify: сервер подписывает transcript своим приватным ключом → доказывает, что владеет сертификатом) — иначе MITM.
3. **Согласовать общий секрет** (ECDHE `key_share` в ClientHello и ServerHello) → из него обе стороны выводят ключи, и **уже `ServerHello` — последнее сообщение открытым текстом** в TLS 1.3.

Всё остальное в задании — про то, где в байтах живёт каждая из этих трёх задач.

### Инструменты и настройка (это и есть «с чего начать»)

Расшифровка — обязательна, иначе TLS 1.3 после `ServerHello` покажет только `Application Data`. Механизм: браузер/`curl` пишут session keys в файл, Wireshark их подхватывает.

```bash
# 1. Терминал: включить дамп ключей и одновременно снимать трафик.
#    tshark пишет pcapng, curl в ТОМ ЖЕ шелле кладёт ключи в keys.log.
export SSLKEYLOGFILE=/tmp/tls-keys.log

# 2. Захват (отдельный терминал или tshark в фоне). en0 — Wi-Fi на mac; проверь `ifconfig`.
sudo tshark -i en0 -w /tmp/tls.pcapng -f "host example.com and tcp port 443"

# 3. В первом терминале — вызвать handshake:
curl -v --http1.1 https://example.com > /dev/null   # http1.1 → ALPN покажет http/1.1
curl -v https://example.com > /dev/null              # без флага → ALPN обычно h2
#    (curl наследует SSLKEYLOGFILE и пишет туда keys)

# 4. Открыть в Wireshark:  wireshark /tmp/tls.pcapng
#    Wireshark → Preferences → Protocols → TLS →
#      (Pre)-Master-Secret log filename = /tmp/tls-keys.log
#    Теперь строка "Application Data" развернётся в Certificate/Finished и т.д.
```

Полезные display-фильтры в Wireshark:

```
tls.handshake                       # только handshake-сообщения
tls.handshake.type == 1             # Client Hello
tls.handshake.type == 2             # Server Hello
tls.handshake.type == 11            # Certificate (виден только после decrypt в 1.3)
tls.handshake.extensions_server_name    # SNI
tls.handshake.extensions_alpn_str        # ALPN
```

### Алгоритм (пошагово)

Разбирай сверху вниз, для каждого пункта — раскрывай дерево в Wireshark (нижняя панель) и находи конкретное поле.

1. **TCP до TLS.** Убедись, что TLS стартует **после** завершённого 3-way handshake (SYN/SYN-ACK/ACK) — TLS живёт поверх установленного TCP-потока, не параллельно. Найти по `tcp.flags`.
2. **Client Hello (cleartext).** Раскрой и найди: `Random` (32 байта), список `Cipher Suites`, extension `supported_versions` (вот **настоящая** максимальная версия — 1.3), `key_share` (эфемерный ECDHE-публичный ключ клиента), `server_name` (SNI), `application_layer_protocol_negotiation` (ALPN — список `h2`, `http/1.1`), `signature_algorithms`. Поле — `tls.handshake.type == 1`.
3. **Легаси-ловушка версии.** Посмотри поле `Version` в записи TLS и в теле ClientHello — оно скажет `TLS 1.2` даже для 1.3-соединения. Реальная версия — **только** в extension `supported_versions`. Понять, почему так (обратная совместимость с middleboxes). Поле — `tls.handshake.extensions.supported_version`.
4. **Server Hello (cleartext).** Найди: серверный `Random`, **выбранный** `Cipher Suite` (один, не список), серверный `key_share`. После этого сообщения обе стороны имеют общий ECDHE-секрет → выводят handshake-ключи. Поле — `tls.handshake.type == 2`.
5. **Граница шифрования.** Без decrypt: замечаешь, что сразу за ServerHello идёт `Application Data` — это уже зашифрованный handshake. Включи keylog (настройка выше) и увидишь, как та же строка разворачивается. Зафиксируй факт: **в TLS 1.3 сертификат сервера НЕ виден пассивному наблюдателю** (в отличие от 1.2).
6. **Encrypted Extensions + Certificate (после decrypt).** Найди `Certificate` (`type == 11`): цепочка leaf → intermediate (root **не** шлётся — он у клиента в trust store). Посмотри в leaf-сертификате `subjectAltName` — там должен быть хост, который ты запрашивал.
7. **CertificateVerify + Finished.** `CertificateVerify` — подпись сервера над всем transcript'ом (доказательство владения приватным ключом). `Finished` — HMAC над transcript, по одному в каждую сторону: проверка, что handshake никто не подменил. Понять словами, что каждое из двух проверяет.
8. **Application Data.** Первый настоящий HTTP-запрос — уже внутри шифра. Если ALPN был `h2`, увидишь HTTP/2 после decrypt; если `http/1.1` — привычный `GET`. Связать ALPN из шага 2 с тем, что реально поехало.

## Контрольные вопросы (аналог тестов — на них надо уметь ответить)

- **Q1 — negotiation.** Где в захвате лежит *реально согласованная* версия TLS и почему нельзя верить полю `Version` в заголовке записи?
- **Q2 — SNI и приватность.** SNI передаётся открытым текстом даже в TLS 1.3. Что из этого может извлечь пассивный наблюдатель между тобой и сервером, и как это используется для фильтрации/цензуры? Что такое ECH и какую дыру он закрывает? (security-фокус)
- **Q3 — аутентификация.** Чем `Certificate` отличается от `CertificateVerify` по роли? Почему одного присланного сертификата недостаточно, чтобы доверять серверу — что именно доказывает подпись в `CertificateVerify`?
- **Q4 — ALPN.** Как по одному handshake-сообщению узнать, поедет HTTP/1.1 или HTTP/2, ещё до первого запроса? Зачем это класть в TLS, а не решать после?
- **Q5 — 1.2 vs 1.3.** Почему в TLS 1.3 ты **не видишь** сертификат сервера без keylog, а в 1.2 увидел бы в открытом виде? Что поменялось и сколько RTT занимает handshake в каждой версии?
- **Q6 — привязка к блоку.** Почему баннер-граббер из B-13 (`recv()` сразу после `connect()`) на порту 443 не получит ничего осмысленного, а на 22/25/21 — получает приветствие сервиса?

---

## Разбор /check

Задание observational (кода нет) — `/check` не проводился. Вместо ревью кода — самопроверка по контрольным вопросам Q1–Q6 (см. выше) и разбор живого захвата в Wireshark. Конспект темы: `topics/networking/tls.md`.

## Session-debrief / итог

**Формат сессии:** `/theory` режим A (мини-лекция по 5 концепциям ДО наблюдения) → живой захват `curl https://example.com` в Kali VM (Wireshark + `SSLKEYLOGFILE` decrypt) → разбор handshake по байтам (8 шагов) → самопроверка Q1–Q6.

**Что усвоено (продемонстрировано ответами):**
- server-first vs client-first: почему `recv()` на 443 виснет, а на 22/25/21 даёт баннер (связал с [[service-fingerprint]]).
- ALPN: клиент предлагает список, сервер выбирает один в EncryptedExtensions — 0 лишних RTT.
- SNI cleartext + почему (сертификат выбирается до появления шифра); SNI-блокировка и ECH как обход, гонка вооружений через DNS-bootstrap.
- Certificate vs CertificateVerify (proof of possession), self-signed никто не доверяет, chain of trust (issuer=subject).
- Version-ловушка (supported_versions), reassembly (record ≠ TCP-сегмент), маскировка content type.

**Сложное место сессии:** асимметричная криптография — пара ключей, что значит sign/verify, почему успешный verify = владение приватным ключом. Разобрано аналогией королевской печати (перстень=private / оттиск=public). ECH-механизм (ключ из DNS HTTPS-record) не знал — дозаполнено. В конспекте на этом сделан акцент.

**Слабые места на будущее:** глубина ECDLP/эллиптических кривых не трогалась (для B достаточно интуиции «нельзя разделить смесь»); детали X.509-полей (extensions сертификата) только обзорно.

**Прямая связь с будущим:** ECDHE (блок 1) = ECDH-handshake VPN-проекта (этап 2), AEAD (блок 3) = ChaCha20-Poly1305 WireGuard-like туннеля. WireGuard-handshake — упрощённый TLS без CA (static keys). См. [[project-vpn-architecture-role]].

### Ключевые термины (English)

Полный глоссарий — в `topics/networking/tls.md` → «Ключевые термины (English)». Ядро: handshake, ClientHello/ServerHello, cipher suite, ECDHE, key_share, forward secrecy, asymmetric key pair, digital signature (sign/verify), Certificate, CertificateVerify, Finished, CA, chain of trust, trust store, record layer, AEAD, SNI, ALPN, ECH, supported_versions, protocol ossification.
