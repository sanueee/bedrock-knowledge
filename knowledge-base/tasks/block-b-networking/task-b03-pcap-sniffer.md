---
task: task-b03
title: pcap sniffer — Ethernet → IP → TCP
status: выполнено
date: 2026-05-25
---

## Задание

Утилита `./sniffer <interface>` — захватывает live-пакеты через libpcap, фильтрует TCP через BPF, парсит Ethernet → IP → TCP слои, печатает по строке на пакет:

```
<ts>  <src_mac> -> <dst_mac>  <src_ip>:<src_port> -> <dst_ip>:<dst_port>  flags=<SYN|ACK|FIN|RST|PSH|URG>  len=<payload_bytes>
```

Требования: BPF-фильтр компилируется и заливается в ядро (а не ручная проверка в callback'е); переменная длина IP и TCP заголовков (`ip_hl * 4`, `th_off * 4`), не хардкод `+20`; SIGINT через `pcap_breakloop` + `pcap_stats` на выходе; чистая компиляция с `-Wall -Wextra`.

Подготовка: `sudo apt install libpcap-dev`. Сборка: `gcc -Wall -Wextra -O2 sniffer.c -lpcap -o sniffer`. Запуск: `sudo ./sniffer lo` + `echo_server`/`echo_client` в соседнем терминале.

---

## Моё решение

Линейный pipeline в `main`: `pcap_open_live` → `pcap_compile("tcp")` + `pcap_setfilter` + `pcap_freecode` → `sigaction(SIGINT)` → `pcap_loop(-1, packet_cb, NULL)` → `pcap_stats` → `pcap_close`. Глобальный `static pcap_t *g_handler` чтобы SIGINT handler мог дёрнуть `pcap_breakloop`. `sa_flags = 0` без `SA_RESTART` — нужно чтобы `recvfrom` внутри loop'а получил `EINTR` и проверил breakloop-флаг.

В `packet_cb` — три cast'а по слоям с проверкой `caplen` перед каждым:
- Ethernet: `bytes` → `struct ether_header *`
- IP: `bytes + sizeof(ether_header)` → `struct ip *`
- TCP: `(u_char *)ip + ip->ip_hl * 4` → `struct tcphdr *`

Cast в `u_char *` перед арифметикой — чтобы `+ip_hl*4` означало байты, а не `sizeof(struct ip)*ip_hl*4` (классическая ловушка pointer arithmetic).

Сборка флагов через массив `{ mask, name }` + цикл с `snprintf(buf+off, sizeof(buf)-off, "%s%s", sep, name)` и `off += n`. На первой итерации сепаратор пустая строка, дальше `"|"`.

## Ключевые функции

| Функция | Зачем |
|---|---|
| `pcap_open_live(dev, snaplen, promisc, to_ms, errbuf)` | Открыть live capture. errbuf нужен потому что handle ещё не существует, некуда хранить ошибку. |
| `pcap_compile` + `pcap_setfilter` | Скомпилировать BPF expression в bytecode и залить в ядро. Дальше ядро отсеивает пакеты до доставки в userspace — win по производительности. |
| `pcap_loop(h, cnt, cb, user)` | Блокирующий цикл захвата. cnt=-1 → бесконечно. user пробрасывается в callback без интерпретации (механизм передачи состояния без глобалов). |
| `pcap_breakloop(h)` | Async-signal-safe выход из loop'а. Единственный корректный способ остановить захват из signal handler'а. |
| `pcap_stats(h, &st)` | ps_recv (доставлено в callback), ps_drop (отброшено ядром из-за переполнения буфера). На быстрых пакетах ps_drop > 0 = signal что callback тормозит. |
| `inet_ntop(AF_INET, &in_addr, buf, len)` | IPv4 в строку. Reentrant, в отличие от `inet_ntoa` (статический буфер → нельзя дважды в одном printf). |
| `ntohs` / `ntohl` | Network byte order (big-endian) → host. Обязательно для `ip_len`, `th_sport`, `th_dport`, `ether_type`. `in_addr` через `inet_ntop` сам разбирается. |

## Что узнал

**libpcap как абстракция.** Главное узкое место сессии — собрать ментальную модель: kernel держит ring buffer захваченных пакетов (на Linux через AF_PACKET socket), BPF фильтр сидит в ядре и отсекает на лету, libpcap из userspace выгребает уцелевшее и зовёт мой callback. Я пишу только "что делать с пакетом", всё остальное — внутри библиотеки и ядра. errbuf vs pcap_geterr — следствие этой модели: до создания handle ошибка некуда сохраняться, после — уже есть куда.

**Переменная длина заголовков и self-describing форматы.** IP и TCP заголовки имеют `IHL` / `Data Offset` поля (4 бита, в 4-байтных словах — отсюда `* 4`). Длина зашита в сам заголовок, потому что опции переменной длины. `sizeof(struct ip)` — компилятор-время, всегда 20, ломается на пакетах с IP options. Идиома `(u_char *)header + length_field * scale` универсальна для бинарных протоколов.

**Pointer arithmetic vs byte arithmetic.** `(struct ip *)p + 1` сдвигает на `sizeof(struct ip) = 20` байт. `(u_char *)p + 1` сдвигает на 1 байт. Перед любым смещением по байтам — cast в `u_char *`. Без него `ip + ip_hl*4` уезжает на `20 * 5 * 4 = 400` байт.

**caplen vs len vs ip_len.** Три разных "длины": `h->caplen` (в моём буфере), `h->len` (на проводе), `ntohs(ip_len)` (по IP-заголовку). caplen — для проверок out-of-bounds. ip_len — для смыслового подсчёта payload'а. Ethernet padding на маленьких пакетах добивает frame до 60 байт → caplen может быть больше реальной длины IP-пакета.

**SA_RESTART наоборот.** Привык в прошлых задачах ставить `SA_RESTART` чтобы syscall'ы не получали EINTR. Здесь нужно противоположное: `pcap_loop` сидит в `recvfrom`, и **нужно** чтобы он получил EINTR на SIGINT → проснулся → проверил breakloop. `sa_flags = 0` без `SA_RESTART` — это правильный выбор для кооперативного завершения через breakloop.

**snprintf с offset и возвращаемое значение.** `snprintf` возвращает "сколько **хотел бы** записать" без `\0`, не "сколько реально записал". Если `n >= sizeof(buf) - off` — буфер переполнен, надо обрезать. Прибавлять к offset нужно `n`, не `strlen(name)` — иначе теряется сепаратор и следующая запись затрёт хвост предыдущей.

## Ошибки и трудности

**Главная сложность по словам самого пользователя:** "научиться работать с пакетами в Linux через библиотеку". То есть концептуально — что захват это конвейер ядро ↔ библиотека ↔ мой callback, и где проходят границы.

**L24 первой итерации — инвертированная логика:** `if (ntohs(ether_type) == ETHERTYPE_IP) return;` (нужно `!=`). pcap_stats показывал `received=32`, но ни одной строки от printf — все 32 пакета вылетали в return. Урок: статистика lib ≠ статистика твоей логики, проверяй на видимом выводе callback'а.

**L69 итерации 2 — `offset += strlen(name)` вместо `offset += n`.** snprintf пишет `"|ACK\0"` (4 байта) но `strlen("ACK") = 3`. offset отстаёт на 1, следующий snprintf затирает последний символ предыдущего имени. Получалось `SYN|AC|PSH` вместо `SYN|ACK|PSH`. Снова не различил "сколько я записал в буфер" vs "сколько символов в исходной строке".

**Мёртвая переменная `mode`.** Попытался завести branching по имени интерфейса (eth0 vs lo), хотя task этого не просит и libpcap сама принимает любую строку. Дважды попала в ревью пока не убрал.

**Защита от обрезанного IP.** Проверка `caplen >= sizeof(ether_header)` была, но между ней и чтением `ip_hl` не было промежуточной проверки `caplen >= sizeof(ether) + sizeof(struct ip)`. Технически UB на коротких пакетах. Допилил после ревью.

## Что бы сделал иначе

- Сразу писать каркас с пустым callback'ом и `printf("packet %u bytes\n", h->len)`, убедиться что приходит — а уже потом ввязываться в парсинг. Я начал почти так, но `mode` влез до того как первый запуск прошёл.
- Тестировать вывод callback'а **в первой же итерации** (не только pcap_stats). Это бы сразу подсветило баг L24.
- Снова всплыла семантика "сколько я хотел" vs "сколько реально" — для snprintf уже второй раз. Запомнить навсегда: возвращаемое n = "сколько хотел", advance используй то же n но через `min(n, remaining-1)`.

## Код

`networking/sniffer.c` (Linux VM через VMware Fusion, libpcap-dev + Ubuntu Server, синхронизация через GitHub).

## Связанные темы

[[libpcap]] [[ethernet-frame]] [[tcp-sockets]] [[proc-net]] [[async-signal-safe]] [[signals-linux]] [[bitwise-flags]]
