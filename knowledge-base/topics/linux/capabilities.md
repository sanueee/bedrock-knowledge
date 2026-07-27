---
тема: Linux capabilities + raw sockets / CAP_NET_RAW, least privilege
блок: B — Сетевой стек
дата: 2026-07-20
связано:
  - "[[syscalls-linux]]"
  - "[[fd-kernel-model]]"
  - "[[tcp-sockets]]"
  - "[[interface-enumeration]]"
---

# Linux capabilities + raw sockets / CAP_NET_RAW

## Что это
Capabilities — дробление всемогущества root на ~40 независимых битов-прав; вместо «root или нет» процессу можно выдать ровно одно нужное право. `CAP_NET_RAW` — право открыть raw/packet socket (`SOCK_RAW`), который отдаёт приложению построение заголовков и потому позволяет спуфинг/сниффинг/инъекцию пакетов.

## Ключевые термины (English)
- **capability** — один бит привилегии из разбитого root (напр. `CAP_NET_RAW`, `CAP_NET_BIND_SERVICE`, `CAP_SYS_ADMIN`, `CAP_SETPCAP`).
- **raw socket** — `socket(AF_INET, SOCK_RAW, ...)`: ядро отдаёт сетевой/транспортный слой, заголовки строит приложение.
- **permitted set (CapPrm)** — «склад»: что процессу *разрешено иметь*; потолок для effective.
- **effective set (CapEff)** — «в руках»: что действует прямо сейчас; **этот набор ядро проверяет в момент syscall**.
- **inheritable set (CapInh)** — что переживёт `execve()` в новую программу.
- **bounding set** — жёсткий потолок на всё дерево процессов; сброс требует `CAP_SETPCAP`.
- **file capabilities** — xattr `security.capability` на бинарнике (`setcap`), из которого процесс получает права при запуске.
- **least privilege / minimum capability surface** — держать минимум прав минимальное время.

## Как работает

**Наборы у процесса** (поля `Cap*` в `/proc/<pid>/status`):
- permitted — потолок; убрал из permitted → в effective уже не поднять.
- effective — действует сейчас; ядро сверяет **его** при syscall.
- inheritable — передаётся через `execve`; по умолчанию пуст (право не протекает в дочерние).
- Мнемоника: **permitted = склад, effective = в руках**. В руки берёшь только со склада; выбросил со склада — из рук пропало и назад не взять.

**Проверка права — в момент syscall, один раз.** `socket(..., SOCK_RAW, ...)` проверяет `CAP_NET_RAW` в effective. Дескриптор выдан → операции над ним (`send`/`recv`/`close`) право **не перепроверяют**. Отсюда паттерн minimum capability surface:
1. открыть raw socket (пока право есть),
2. сразу сбросить capability,
3. дальше работать с уже открытым fd — новый raw socket открыть уже нельзя. Окно, в котором процесс вооружён опасным правом, сжато до одной строки; RCE после сброса `CAP_NET_RAW` уже не даст.

**Три способа выдать право:**

| Способ | Что получает процесс | Оценка |
|--------|----------------------|--------|
| `sudo ./prog` (root) | euid=0 + **все** capabilities | грубо — всё ради одного |
| `chmod u+s` (setuid-root) | euid=0 + **все** capabilities | ещё хуже: эскалатор при каждом запуске |
| `setcap cap_net_raw+ep ./prog` | uid обычный, **одна** capability | least privilege ✅ |

`+ep` = положить право в файловые наборы **e**ffective + **p**ermitted. `p` → permitted процесса при `execve`; `e` (один бит на весь файл) → «сразу подними permitted в effective», чтобы наивная утилита не звала `capng_update` сама. Без `e` право лежало бы в permitted мёртвым грузом.

## Пример
`sections/networking/raw-socket-caps/rawcap.c` (B-17): открыть raw socket → сбросить cap → доказать, что старый fd жив, а новый `socket()` даёт `EPERM`.

```c
// libcap-ng: буфер библиотеки vs процесс
capng_get_caps_process();                              // загрузить состояние в буфер (ПЕРВЫМ)
int res = capng_have_capability(CAPNG_EFFECTIVE, CAP_NET_RAW); // 1/0 по буферу
int fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);      // ядро проверяет CapEff здесь
// ... fd открыт ...
capng_clear(CAPNG_SELECT_CAPS);                        // ТОЛЬКО буфер, void
if (capng_apply(CAPNG_SELECT_CAPS) < 0) { /* fail */ } // apply реально роняет права процесса
```

Три сценария в Docker (все проходят): nobody без caps → `EPERM`; root → полный путь; `setcap cap_net_raw+ep` + nobody → полный путь под uid≠0.

## Подводные камни
- **`capng_clear` — `void`, меняет только буфер библиотеки.** Без `capng_apply` права процесса не меняются: сброс фиктивный, второй `socket()` спокойно откроется. Классический баг «сбросил и обрадовался».
- **`capng_get_caps_process()` обязателен ПЕРВЫМ.** `capng_have_capability` читает **буфер**, не процесс напрямую; без загрузки судишь о пустом буфере.
- **`CAPNG_SELECT_BOTH` рушит целевой сценарий (setcap + non-root).** `_BOTH` = capabilities + bounding set; сброс bounding set требует `CAP_SETPCAP`, которого у процесса с одной `cap_net_raw+ep` нет → `capng_apply` возвращает `< 0`. Под root проходит (там `CAP_SETPCAP` есть), под nobody — падает. Для сброса самого права нужен **`CAPNG_SELECT_CAPS`** (bounding трогать не надо и права нет). Пойман только прогоном Теста 3 — по чтению `_BOTH` выглядел «полнее».
- **file capabilities = xattr `security.capability`.** Bind-mount с macOS-хоста их не сохраняет → `setcap` на файле в смонтированном томе не сработает; собирать/класть бинарник в контейнерную ФС.
- **Порядок stdout/stderr в выводе.** stderr небуферизован, stdout при пайпе — полностью буферизован → строки из stderr всплывают выше stdout. Логика верна, вывод перепутан; лечится `fflush(stdout)` перед печатью в stderr.
- **raw socket = мощное право.** Спуфинг source IP (reflection/amplification-DoS), сниффинг, инъекция пакетов — потому и вынесено в отдельную gated capability.

## Связанные темы
[[syscalls-linux]] [[fd-kernel-model]] [[tcp-sockets]] [[interface-enumeration]]
