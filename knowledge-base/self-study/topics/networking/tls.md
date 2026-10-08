---
тема: TLS 1.3 handshake — рукопожатие / TLS handshake
блок: B — Сетевой стек
дата: 2026-07-14
связано:
  - "[[service-fingerprint]]"
  - "[[dns]]"
  - "[[tcp-sockets]]"
  - "[[http]]"
  - "[[libpcap]]"
---

# TLS 1.3 handshake / TLS handshake

## Что это

TLS (Transport Layer Security) — слой между TCP и приложением (HTTP), который поверх установленного TCP-потока строит **шифрованный, целостный, аутентифицированный** канал. Handshake (рукопожатие) — это обмен сообщениями **до первого прикладного байта**, который решает три задачи разом: договориться о параметрах, аутентифицировать сервер, согласовать общий секрет. Наблюдательное задание B-14: снять живой HTTPS в Wireshark и разобрать handshake по байтам.

## Ключевые термины (English)

- **handshake** — фаза установления TLS до передачи данных.
- **ClientHello / ServerHello** — первые сообщения (клиент предлагает, сервер выбирает).
- **cipher suite** — именованный набор алгоритмов соединения. В 1.3 короткий: AEAD-шифр + hash (`TLS_AES_256_GCM_SHA384`). В 1.2 длинный: key-exchange + auth + cipher + hash.
- **ECDHE (Elliptic Curve Diffie-Hellman Ephemeral)** — согласование общего секрета через открытый канал; E = эфемерный (новые ключи на каждое соединение).
- **key_share** — публичная половинка ECDHE в ClientHello и ServerHello.
- **forward secrecy** — украли долгосрочный ключ сервера позже → старый трафик не расшифровать (эфемерные ключи сессий уничтожены).
- **asymmetric cryptography / key pair** — пара связанных чисел: public (открытый) + private (секретный); из private легко получить public, обратно — невозможно.
- **digital signature (sign / verify)** — sign: обработать данные приватным ключом → подпись. verify: проверить (данные + подпись + публичный ключ) → да/нет. Успешный verify = подписант владел приватным ключом.
- **Certificate** — документ «публичный ключ K принадлежит X», подписанный CA. Сам по себе ничего не доказывает (публичный, копируется).
- **CertificateVerify** — подпись сервером transcript'а приватным ключом → доказательство владения ключом (proof of possession). Против MITM.
- **Finished** — HMAC над transcript'ом, по одному в каждую сторону: handshake не подменён.
- **CA (Certificate Authority)** — удостоверяющий центр; проверил владение доменом и подписал сертификат.
- **chain of trust** — цепочка leaf → intermediate → root; `issuer` одного = `subject` следующего; root не шлётся (в trust store клиента).
- **trust store** — предустановленное хранилище доверенных корневых сертификатов.
- **record layer / TLS record** — «конвертный» слой: всё (handshake, data, alerts) заворачивается в записи `[content type][version][length][payload]`.
- **AEAD (Authenticated Encryption with Associated Data)** — шифрование + authentication tag одной операцией: конфиденциальность + целостность. Любое изменение бита → tag не сходится → запись отвергается.
- **SNI (Server Name Indication)** — имя хоста в ClientHello открытым текстом (сервер выбирает сертификат при virtual hosting).
- **ALPN (Application-Layer Protocol Negotiation)** — выбор протокола поверх TLS (h2 / http/1.1) внутри handshake, без лишних RTT.
- **ECH (Encrypted Client Hello)** — шифрует ClientHello (вкл. SNI) отдельным публичным ключом сервера, полученным заранее через DNS (HTTPS/SVCB record).
- **supported_versions** — extension с настоящей версией TLS (поля Version врут `1.2` ради middlebox-совместимости).
- **middlebox / protocol ossification** — промежуточные узлы на пути ломаются на незнакомых значениях → 1.3 маскируется под 1.2 в старых полях.

## Как работает

**Три задачи handshake (центральная идея):**
1. **Negotiation** — договориться о версии TLS, cipher suite, ECDHE-группе.
2. **Authentication** — доказать, что на том конце реально `example.com` (Certificate + CertificateVerify).
3. **Key agreement** — вывести общий секрет (ECDHE key_share) → из него ключи; **ServerHello — последнее сообщение открытым текстом** в 1.3.

**Поток TLS 1.3 (1-RTT), как в живом захвате:**
1. TCP 3-way handshake (SYN/SYN-ACK/ACK) — TLS живёт поверх установленного потока.
2. **ClientHello** (cleartext): Random, список cipher suites, `supported_versions` (=1.3), `key_share` клиента, SNI, ALPN, signature_algorithms.
3. **ServerHello** (cleartext): Random, **выбранный** cipher suite (один), `key_share` сервера. После него обе стороны имеют общий ECDHE-секрет → выводят handshake-ключи → **дальше всё шифровано**.
4. **{EncryptedExtensions}** (шифр): результат ALPN и др.
5. **{Certificate}** (шифр): цепочка сертификатов сервера.
6. **{CertificateVerify}** (шифр): подпись transcript'а приватным ключом сервера.
7. **{Finished}** (шифр): HMAC над transcript, обе стороны.
8. **Application Data** (шифр): реальный HTTP-запрос (HTTP/2 если ALPN=h2).

**ECDHE — как договориться о секрете через открытый канал.** Аналогия смешивания красок: есть публичная `G`; клиент выбирает секрет `a`, шлёт `G+a` (key_share); сервер выбирает `b`, шлёт `G+b`. Клиент считает `(G+b)+a`, сервер `(G+a)+b` → `G+a+b` у обоих. Наблюдатель видит `G`, `G+a`, `G+b`, но «разделить» смесь не может — это **discrete logarithm problem** (для эллиптических кривых — ECDLP): по точке `G+a` не найти `a`. Секрет **не передаётся**, а независимо вычисляется. Ephemeral → forward secrecy.

**Асимметричная криптография и подпись (было самым сложным в сессии).** Пара ключей — два математически спаянных при рождении числа. Private держит сервер в секрете; public лежит в сертификате, видят все. Свойство асимметрии:
- **sign**: данные + **private** → подпись. Произвести может только владелец private.
- **verify**: данные + подпись + **public** → да/нет. Проверить может кто угодно.
- Verify проходит ⟺ подпись сделана private-ключом, парным этому public. Подделать подпись, зная лишь public, невозможно.

Аналогия — королевская печать: private = сам перстень (один, у короля), public = как выглядит оттиск (знают все). Sign = придавить перстнем (может только король). Verify = узнать оттиск (может любой). Сфотографировать оттиск ≠ мочь ставить новые печати. Отсюда: успешный verify `CertificateVerify` = сервер **владеет** приватным ключом сейчас, а не скопировал публичный сертификат → MITM отсекается (у него нет private-ключа `example.com`).

**Chain of trust.** В сертификате X.509: `subject` (чей), `issuer` (кто подписал), `validity` (срок), `subjectPublicKeyInfo` (тот самый публичный ключ, которым проверяется CertificateVerify), `signature` (алгоритм, которым CA подписал этот cert). Цепочка сцеплена правилом **`issuer` одного = `subject` следующего**: leaf ← intermediate ← … ← root. Root **не шлётся** — он в trust store клиента. Self-signed (issuer=subject=сам) никто не доверяет: так может заявить кто угодно, CA же **проверяет** владение доменом до подписи.

**Record layer + AEAD.** TLS = два под-протокола поверх одного TCP: handshake protocol (согласует ключи) + record protocol (конвертный слой). Всё заворачивается в record: `[content type (22=handshake, 23=app_data, 21=alert)][version][length][payload]`. Один record ≠ один TCP-сегмент (framing по `length`, как HTTP Content-Length — Wireshark делает reassembly). Payload защищён AEAD: шифрование + authentication tag. Меняешь хоть бит → tag не сходится → запись отвергается, соединение рвётся `bad_record_mac`. В 1.3 внешний `content type` всех шифрованных записей маскируется под `23 (Application Data)`, реальный тип — внутри шифра.

**SNI и ALPN (extensions ClientHello).** SNI — имя хоста cleartext, т.к. сервер выбирает сертификат до появления шифра (virtual hosting: сотни сайтов за одним IP). ALPN — клиент перечисляет протоколы (h2, http/1.1), сервер выбирает один (в 1.3 — в зашифрованных EncryptedExtensions); версия HTTP решена внутри handshake, 0 лишних RTT.

**1.2 vs 1.3.** 1.2: 2 RTT, Certificate **cleartext**. 1.3: клиент кладёт key_share уже в ClientHello → секрет готов сразу после ServerHello → 1 RTT, Certificate **внутри шифра**. ECDHE обязателен (forward secrecy по умолчанию), выкинуты слабые cipher suites.

## Пример

Живой захват `curl https://example.com` в Kali (Wireshark + `SSLKEYLOGFILE` для decrypt):

```
5-8    DNS      AAAA/A example.com → 104.20.23.154 (за Cloudflare)
9-11   TCP      SYN / SYN-ACK / ACK
12     TLSv1.3  Client Hello (SNI=example.com), 90 cipher suites
15     TLSv1.3  Server Hello, Change Cipher Spec  ← дальше шифр
19/41  TLSv1.3  Encrypted Extensions, Certificate, Certificate Verify, Finished
23     HTTP2    Magic, SETTINGS, HEADERS[1]: GET /   ← ALPN выбрал h2
34     TLSv1.3  Alert (Close Notify)
```

Ловушка версии (ClientHello, пакет 12):
```
Record Layer → Version: TLS 1.0 (0x0301)     ← врёт
Handshake    → Version: TLS 1.2 (0x0303)     ← тоже врёт
supported_versions extension → TLS 1.3 (0x0304)  ← ПРАВДА
```

Cipher suite (выбран сервером): `TLS_AES_256_GCM_SHA384 (0x1302)` — AEAD `AES-256-GCM` + hash `SHA384`.

Reassembly (пакет 41 = Certificate record 3861 байт):
```
[3 Reassembled TCP Segments: #37(1659), #39(1440), #41(767)]
Opaque Type: Application Data (23)     ← маскировка снаружи
[Content Type: Handshake (22)]         ← реальный тип внутри шифра
```

Chain of trust (Certificate, 4 сертификата):
```
Certificate #1 (leaf): subject rdnSequence → commonName=example.com
   signature (ecdsa-with-SHA256), issuer, validity, subjectPublicKeyInfo
Certificate #2..#4: промежуточные CA (issuer[N] = subject[N+1])
CertificateVerify: Signature Algorithm ecdsa_secp256r1_sha256, Signature 3046...
Finished: Verify Data 07baefe...
```

## Подводные камни

- **Поле Version врёт.** Настоящая версия — только в `supported_versions`. Не верить полям Version в Record/Handshake (там `1.2` даже для 1.3).
- **Без keylog в 1.3 не видно ничего после ServerHello.** Certificate/Finished зашифрованы, показываются как `Application Data`. Нужен `SSLKEYLOGFILE` + путь в Wireshark → TLS preferences. Захват и `curl` — на **одной** машине (ключи привязаны к сессии).
- **`recv()` сразу после `connect()` на 443 повиснет.** HTTPS — client-first (сервер ждёт ClientHello), в отличие от server-first SSH/SMTP/FTP (см. [[service-fingerprint]]). Дедлок: обе стороны ждут друг друга.
- **Certificate ≠ доказательство.** Публичный сертификат копируется. Доказывает владение только CertificateVerify (подпись). Путать роли — концептуальная ошибка.
- **SNI течёт даже в TLS 1.3.** Пассивный наблюдатель видит хост → SNI-based filtering/censorship. Закрывает ECH, но только в паре с шифрованным DNS (иначе хост течёт из DNS-запроса, см. [[dns]]).
- **Change Cipher Spec в 1.3 — пустышка** (legacy dummy ради middlebox-совместимости), криптографически ничего не значит.
- **AEAD рвёт соединение при tampering.** Флип бита → `bad_record_mac`, вся сессия падает (не «пропустим битую запись»). Наблюдатель максимум устроит DoS, не подмену.
- **Cipher suites в ClientHello — предложения.** `TLS_DH_anon_*`, `_CBC_` (padding-oracle-уязвимые) шлются ради совместимости, но раз сервер выбрал 1.3+GCM — не задействованы. В проде на сервере отключаются.

## Связанные темы

[[service-fingerprint]] (server-first vs client-first, почему 443 не даёт баннер) · [[dns]] (резолв до TCP; ECH-config в HTTPS/SVCB record; SNI vs DNS-утечка) · [[tcp-sockets]] (TLS поверх установленного потока) · [[http]] (HTTP/2 поверх TLS, ALPN) · [[libpcap]] (как Wireshark захватывает: PF_PACKET + BPF)
