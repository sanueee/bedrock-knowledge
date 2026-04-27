# /proc/net/tcp — чтение TCP-соединений

## Что это

`/proc/net/tcp` — виртуальный файл ядра, живой снимок таблицы TCP-соединений. Меняется в реальном времени — каждый `cat` даёт актуальное состояние.

## Формат файла

```
sl  local_address  rem_address  st  tx_queue rx_queue  ...
0:  3600007F:0035  00000000:0000  0A  ...
```

- поле 2 — local IP:port в hex, little-endian
- поле 3 — remote IP:port в hex, little-endian
- поле 4 — state в hex: `01`=ESTABLISHED, `0A`=LISTEN, `06`=TIME_WAIT

## Как парсить

```c
char local[16], remote[16], state[16];
sscanf(line, "%*s %15s %15s %15s", local, remote, state);
// %*s — пропустить первое поле ("0:" или "sl")
```

## Как конвертировать IP

IP хранится в little-endian: байты нужно разворачивать.

```
3600007F → байты: 36 00 00 7F → разворот → 7F 00 00 36 → 127.0.0.54
```

```c
unsigned int ip, port;
sscanf(address, "%X:%X", &ip, &port);

// извлечь байты через маску и сдвиги
(ip)       & 0xFF   // байт 0 → первый октет
(ip >> 8)  & 0xFF   // байт 1
(ip >> 16) & 0xFF   // байт 2
(ip >> 24) & 0xFF   // байт 3 → последний октет
```

`& 0xFF` — маска `11111111`, оставляет только младшие 8 бит, обнуляет остальное.
`>> N` — сдвиг вправо на N бит, нужный байт оказывается на позиции 0.

## Пример полной конвертации адреса

```c
void print_address(char *address) {
    unsigned int ip, port;
    sscanf(address, "%X:%X", &ip, &port);
    char buf[32];
    snprintf(buf, sizeof(buf), "%d.%d.%d.%d:%d",
        (ip)       & 0xFF,
        (ip >> 8)  & 0xFF,
        (ip >> 16) & 0xFF,
        (ip >> 24) & 0xFF,
        port);
    fprintf(stdout, "%-25s", buf);
}
```

## Подводные камни

| Ошибка | Последствие |
|--------|-------------|
| `%*d` вместо `%*s` для пропуска поля | Останавливается перед `:`, парсинг съезжает |
| Не разворачивать байты IP | Адрес выводится задом наперёд |
| Сравнивать `/proc/net/tcp` как статику | Файл меняется между запусками — это норма |

## Коды состояний

```
01 → ESTABLISHED
02 → SYN_SENT
03 → SYN_RECV
04 → FIN_WAIT1
05 → FIN_WAIT2
06 → TIME_WAIT
07 → CLOSE
08 → CLOSE_WAIT
09 → LAST_ACK
0A → LISTEN
0B → CLOSING
```

## Связанные темы

[[proc-filesystem]] [[file-io-c]] [[string-formatting-c]]
