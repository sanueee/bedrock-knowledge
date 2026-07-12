---
тема: libpcap — захват пакетов из user space / Packet capture library
блок: B — Сетевой стек
дата: 2026-05-25
связано:
  - "[[ethernet-frame]]"
  - "[[tcp-sockets]]"
  - "[[async-signal-safe]]"
  - "[[signals-linux]]"
  - "[[fd-kernel-model]]"
---

# libpcap / Packet capture library

## Что это

libpcap — user-space библиотека для перехвата сетевых пакетов. Это обёртка над ядерным механизмом захвата (на Linux — `AF_PACKET` socket + **BPF** virtual machine в ядре; на macOS — `/dev/bpf*`).

Mental model: kernel держит ring buffer захваченных frame'ов, BPF фильтр сидит в ядре и отсекает пакеты до доставки наверх, libpcap из userspace выгребает то что прошло и зовёт мой callback. Я пишу только **что делать с пакетом** — захват, фильтрацию и буферизацию делает связка ядро+библиотека.

## Ключевые термины (English)

- **packet capture** — перехват frame'ов на сетевом интерфейсе
- **BPF (Berkeley Packet Filter)** — мини-VM в ядре, исполняющая filter bytecode на каждом frame'е
- **BPF expression** — строка фильтра ("tcp", "port 80 and host 1.2.3.4"), которую `pcap_compile` превращает в bytecode
- **snaplen (snapshot length)** — лимит на количество захватываемых байт от каждого пакета (65535 = весь)
- **promiscuous mode** — режим NIC при котором карта отдаёт ядру все frame'ы, не только адресованные ей
- **monitor mode** — Wi-Fi-специфичный режим, шире чем promiscuous (захват management/control frame'ов сторонних сетей); не путать с promisc
- **handle** — opaque объект `pcap_t *`, описывающий сессию захвата
- **callback** — пользовательская функция, вызываемая `pcap_loop` на каждый пакет
- **pkthdr (packet header)** — библиотечные метаданные: timestamp, caplen, len
- **caplen vs len** — сколько байт в буфере vs сколько было на проводе (могут различаться если snaplen меньше пакета)
- **ring buffer** — circular буфер в ядре где копятся захваченные пакеты до выборки userspace'ом
- **ps_recv / ps_drop** — статистика: доставлено в callback / отброшено ядром из-за переполнения
- **breakloop** — кооперативный выход из `pcap_loop` (async-signal-safe)
- **freecode** — освобождение скомпилированного BPF bytecode'а в userspace (после загрузки в ядро локальная копия не нужна)

## Как работает

### Жизненный цикл сессии

```
1. pcap_open_live(dev, snaplen, promisc, to_ms, errbuf)
   → создаёт handle (pcap_t *), открывает AF_PACKET socket в ядре

2. pcap_compile(handle, &fp, "tcp", optimize, netmask)
   → BPF expression → bpf_program (bytecode)

3. pcap_setfilter(handle, &fp)
   → заливает bytecode в ядро через SO_ATTACH_FILTER
   → дальше ядро отсеивает пакеты ДО доставки в userspace

4. pcap_freecode(&fp)
   → освобождает локальную копию bytecode'а (ядро уже скопировало)

5. pcap_loop(handle, cnt, callback, user)
   → блокирующий цикл: read из ring buffer → вызов callback
   → cnt=-1 → бесконечно

6. pcap_breakloop(handle)        ← из signal handler'а
   → кооперативный выход из loop'а после текущего пакета

7. pcap_stats(handle, &st)
   → достать ps_recv / ps_drop

8. pcap_close(handle)
```

### Сигнатура callback'а

```c
void packet_cb(u_char *user, const struct pcap_pkthdr *h, const u_char *bytes);
```

- `user` — указатель, который ты сам передал последним аргументом в `pcap_loop`. Не интерпретируется библиотекой, просто пробрасывается. Механизм передачи состояния без глобалов.
- `h` — метаданные пакета (timestamp, caplen, len).
- `bytes` — сырые байты, начиная с Ethernet header'а. **`const`** — это библиотечный буфер, после возврата невалиден.

### Где границы ответственности

| Кто | Что делает |
|---|---|
| **NIC** (железо) | Фильтрует frame'ы по MAC (promiscuous off) или принимает все (promiscuous on) |
| **Ядро** | Копирует frame'ы в ring buffer, исполняет BPF на каждом, отбрасывает не прошедшие фильтр |
| **libpcap** | Выгребает из ring buffer'а через recvfrom/read, парсит pkthdr, зовёт callback |
| **Ты** | Пишешь только callback и main с lifecycle |

## Пример

Фрагмент из `sections/networking/pcap-sniffer/sniffer.c`:

```c
static pcap_t *g_handler = NULL;

static void on_sigint(int sig) {
    if (g_handler) pcap_breakloop(g_handler);
}

g_handler = pcap_open_live(interface, 65535, 1, 1000, errbuf);
struct bpf_program fp;
pcap_compile(g_handler, &fp, "tcp", 1, PCAP_NETMASK_UNKNOWN);
pcap_setfilter(g_handler, &fp);
pcap_freecode(&fp);

struct sigaction sa = { .sa_handler = on_sigint };
sigemptyset(&sa.sa_mask);
sa.sa_flags = 0;   // важно: БЕЗ SA_RESTART
sigaction(SIGINT, &sa, NULL);

pcap_loop(g_handler, -1, packet_cb, NULL);

struct pcap_stat st;
pcap_stats(g_handler, &st);
pcap_close(g_handler);
```

## Подводные камни

- **`errbuf` vs `pcap_geterr`**: `pcap_open_live` принимает `errbuf` потому что handle ещё не существует — ошибку некуда хранить. После создания handle ошибки сохраняются внутрь объекта, достаются через `pcap_geterr(h)`. Указатель из `pcap_geterr` валиден только пока `h` жив — не дёргать после `pcap_close`.
- **`pcap_freecode` сразу после `setfilter`**, не в конце программы. BPF bytecode уже скопирован в ядро, локальная копия больше не нужна — занимает память до конца сессии без пользы.
- **`SA_RESTART` наоборот**. Привычка из других задач — ставить `SA_RESTART` чтобы syscall'ы не получали EINTR. Здесь нужно противоположное: `pcap_loop` блокирует в `recvfrom`, и нужно чтобы он получил EINTR на SIGINT → проснулся → проверил breakloop-флаг → вышел. `sa_flags = 0`.
- **`pcap_breakloop` async-signal-safe**, а почти всё остальное в libpcap — нет. Из signal handler'а можно вызывать только её. Логирование, флаги, что угодно ещё — нельзя.
- **Глобальный handle для signal handler'а**. Указатель не помещается в `sig_atomic_t`, но запись/чтение указателя на правильно выровненный адрес атомарна на x86_64 по факту. `static pcap_t *g_handler` достаточно — проверять `if (g_handler)` перед `pcap_breakloop`.
- **caplen vs len**. `len` — реальная длина пакета на проводе. `caplen` — сколько байт в твоём буфере (ограничено `snaplen`). Если `caplen < len` — пакет обрезан. **Для проверок out-of-bounds использовать `caplen`**, не `len`.
- **`ps_drop > 0`** = твой callback тормозит, ядро не успевает скидывать пакеты в ring buffer. Решение: ускорить callback (минимум I/O), либо увеличить буфер (`pcap_set_buffer_size` — другая API ветка через `pcap_create` + `pcap_activate`).
- **promiscuous mode на switched Ethernet** даёт мало: на современных switch'ах ты всё равно видишь только свой трафик + broadcast/multicast. Чтобы реально захватывать чужое — нужен SPAN port на switch'е или MITM.
- **Права**. `pcap_open_live` требует `CAP_NET_RAW` (или root). Без них либо упадёт, либо тихо проигнорирует promisc-флаг — зависит от системы. Решение: `sudo` или `setcap cap_net_raw,cap_net_admin=eip ./sniffer`.

## Связанные темы

[[ethernet-frame]] [[tcp-sockets]] [[async-signal-safe]] [[signals-linux]] [[fd-kernel-model]]
