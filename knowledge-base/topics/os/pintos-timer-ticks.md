---
тема: Таймер 8254 и счётчик ticks / PIT & timer interrupt (Pintos)
блок: Курс — ОС (Pintos)
дата: 2026-09-14
связано:
  - "[[os-index]]"
  - "[[interrupts]]"
  - "[[pintos-preemption]]"
  - "[[pintos-timer-sleep]]"
  - "[[pintos-timer-calibrate]]"
---

# Таймер 8254 и счётчик ticks / PIT & timer interrupt (Pintos)

## Что это

`ticks` (`devices/timer.c:21`) — это часы ОС: программный счётчик прерываний таймера с момента их включения. Каждое прерывание генерирует микросхема 8254 — программируемый интервальный таймер (PIT, Programmable Interval Timer) — `TIMER_FREQ = 100` раз в секунду, то есть тик = 10 мс.

Три сущности, которые легко перепутать:

| Что | Где | Что это |
|---|---|---|
| микросхема 8254 (PIT) | железо, `devices/pit.c` | генератор, который 100 раз в секунду шлёт прерывание |
| `ticks` | `devices/timer.c:21` | программный счётчик этих прерываний — часы ОС |
| `loops_per_tick` | `devices/timer.c:25` | сколько итераций пустого цикла помещается между двумя тиками — см. [[pintos-timer-calibrate]] |

«Отдельного таймера ОС» кроме `ticks` нет. Счётчики в `thread.c` (`thread_ticks`, `idle_ticks`, …) растут от того же прерывания — см. [[pintos-preemption]].

---

## Ключевые термины (English)

- **PIT (Programmable Interval Timer)** — микросхема Intel 8254, три канала-счётчика с общим генератором
- **oscillator** — кварцевый генератор PIT, `PIT_HZ = 1193180` Гц
- **counter / reload value** — число, от которого канал считает вниз; определяет частоту импульсов
- **rate generator (mode 2)** — режим PIT: периодический короткий импульс, удобен для прерываний
- **square wave (mode 3)** — режим PIT: меандр, используется для динамика (`devices/speaker.c`)
- **tick** — один период таймера, одно прерывание (10 мс при `TIMER_FREQ = 100`)
- **timer interrupt** — прерывание от канала 0 PIT, IRQ0, вектор `0x20`
- **I/O port** — адрес в пространстве ввода-вывода x86, запись через `outb`
- **atomic read** — чтение значения, которое не может быть прервано посередине
- **torn read** — «разорванное» чтение: половина старого значения, половина нового

---

## Как работает

### 1. Железо: 8254

У 8254 есть генератор на `PIT_HZ = 1193180` Гц (`pit.c:15`) и три канала:

- **канал 0** — подключён к IRQ0, периодическое прерывание таймера (`devices/timer.c`);
- **канал 1** — регенерация DRAM в старых PC, трогать нельзя;
- **канал 2** — динамик PC (`devices/speaker.c`).

В канал загружается число `count`. Каждый такт генератора уменьшает его на 1. Когда счётчик доходит до конца, на выходе — импульс, счётчик перезагружается, цикл повторяется. Частота импульсов = `1193180 / count`.

### 2. Настройка: `timer_init`

```c
/* devices/timer.c:36 */
void
timer_init (void) 
{
  pit_configure_channel (0, 2, TIMER_FREQ);           // канал 0, режим 2, 100 Гц
  intr_register_ext (0x20, timer_interrupt, "8254 Timer");
}
```

Внутри `pit_configure_channel` (`devices/pit.c:48`):

```c
count = (PIT_HZ + frequency / 2) / frequency;   // (1193180 + 50) / 100 = 11932 (округление к ближайшему)

old_level = intr_disable ();
outb (PIT_PORT_CONTROL, (channel << 6) | 0x30 | (mode << 1));  // порт 0x43: канал, «младший+старший байт», режим
outb (PIT_PORT_COUNTER (channel), count);        // порт 0x40: младший байт
outb (PIT_PORT_COUNTER (channel), count >> 8);   // порт 0x40: старший байт
intr_set_level (old_level);
```

- `(channel << 6)` — биты 7–6 выбирают канал.
- `0x30` — биты 5–4 = `11`: счётчик загружается двумя записями, сначала младший байт, потом старший.
- `(mode << 1)` — биты 3–1 — режим.
- Прерывания выключены, потому что две записи в порт `0x40` должны идти подряд — иначе PIT примет старший байт за младший.

После этого 8254 **сам, без участия ОС**, каждые ~10 мс выдаёт импульс на IRQ0. IRQ0 через PIC приходит на вектор `0x20` (переназначение в `pic_init`, см. [[interrupts]]).

Защиты на значения частоты (`devices/timer.c:13`):

```c
#if TIMER_FREQ < 19
#error 8254 timer requires TIMER_FREQ >= 19
#endif
#if TIMER_FREQ > 1000
#error TIMER_FREQ <= 1000 recommended
#endif
```

- Нижняя граница — железо: 16-битный счётчик делит максимум на 65536, `1193180 / 65536 ≈ 18.2` Гц.
- Верхняя — накладные расходы: чем чаще тик, тем больше времени уходит на сам обработчик, и тем грубее калибровка `loops_per_tick`.

### 3. Порядок запуска (`threads/init.c`)

```c
intr_init ();        // :109  IDT + переназначение PIC
timer_init ();       // :110  8254 генерирует импульсы, обработчик зарегистрирован
kbd_init ();
input_init ();
...
thread_start ();     // :119  внутри — intr_enable()  (threads/thread.c:114)
serial_init_queue ();
timer_calibrate ();  // :121  прерывания включены, ticks растёт
```

Импульсы идут сразу после `timer_init`, но пока IF = 0, процессор их не принимает. **`ticks` начинает расти только после `intr_enable()` в `thread_start`.** Поэтому `timer_calibrate` стоит после неё и начинается с `ASSERT (intr_get_level () == INTR_ON)`.

### 4. Путь одного тика до `ticks++`

```
8254 канал 0 ── импульс ──> PIC 8259 (IRQ0) ──> CPU: вектор 0x20
                                                    │
     процессор дописывает текущую инструкцию, сбрасывает IF,
     кладёт в стек EFLAGS, CS, EIP и прыгает по IDT[0x20]
                                                    │
intr-stubs.S:  intr20_stub:  push 0 (фиктивный код ошибки)
                             push $0x20 (номер вектора)
                             jmp intr_entry
                                                    │
intr-stubs.S:  intr_entry:   сохраняет ds, es, fs, gs + pushal
                             → struct intr_frame
                             call intr_handler(frame)
                                                    │
interrupt.c:346 intr_handler:
      external = true; in_external_intr = true; yield_on_return = false;
      handler = intr_handlers[0x20]  → timer_interrupt
      handler(frame) ─────────────────────────────┐
                                                  ▼
                              timer.c:171  timer_interrupt:
                                  ticks++;          ← ЗДЕСЬ
                                  thread_tick ();   ← (pintos-preemption)
                                                  │
      ◄───────────────────────────────────────────┘
      in_external_intr = false;
      pic_end_of_interrupt (0x20);   // EOI
      if (yield_on_return) thread_yield ();
                                                    │
intr-stubs.S:  intr_exit:    popal, сегменты, iret → поток продолжает работу
```

### 5. Свойства `ticks`

- **Пишется ровно в одном месте** — `ticks++` в `timer_interrupt` (`timer.c:173`).
- **Единица — один тик** = `1/TIMER_FREQ` с. `N * TIMER_FREQ` — это N секунд в тиках.
- **Инкремент не может быть прерван другим тиком** — внешние обработчики работают с IF = 0.
- **Снаружи читается только через `timer_ticks()`** — переменная `static`.

---

## Пример: чтение `ticks`

```c
/* devices/timer.c:71 */
int64_t
timer_ticks (void) 
{
  enum intr_level old_level = intr_disable ();
  int64_t t = ticks;
  intr_set_level (old_level);
  return t;
}

/* devices/timer.c:82 */
int64_t
timer_elapsed (int64_t then) 
{
  return timer_ticks () - then;
}
```

**Почему выключаются прерывания.** Pintos — 32-битный x86, `int64_t` занимает два машинных слова, копирование — две инструкции `mov`. Если тик придёт между ними на переходе `0x00000000FFFFFFFF → 0x0000000100000000`:

```
прочитали младшее слово:  0xFFFFFFFF   (старое)
      ── прерывание: ticks++ ──
прочитали старшее слово:  0x00000001   (новое)
результат:                0x00000001FFFFFFFF   ← на 4 млрд тиков больше реального
```

Использование в тестах:

```c
/* tests/threads/alarm-priority.c:24 — разбудить через 5 секунд */
wake_time = timer_ticks () + 5 * TIMER_FREQ;
...
timer_sleep (wake_time - timer_ticks ());

/* tests/threads/mlfqs-block.c:39 — крутиться 5 секунд */
start_time = timer_ticks ();
while (timer_elapsed (start_time) < 5 * TIMER_FREQ)
  continue;
```

Статистика при выключении (`devices/shutdown.c:126`):

```c
void
timer_print_stats (void) 
{
  printf ("Timer: %"PRId64" ticks\n", timer_ticks ());
}
```

---

## Карта: кто читает `ticks`

```
                           ticks  (пишет только timer_interrupt)
                             │
             ┌───────────────┼──────────────────────────────┐
             │               │                              │
       timer_ticks()   too_many_loops()               (после Alarm Clock:
             │         (напрямую — калибровка)          timer_interrupt будит
             │                                          спящих по ticks)
   ┌─────────┼──────────────┬──────────────────┐
timer_     timer_sleep   timer_print_stats   тесты, MLFQS
elapsed()
```

---

## Подводные камни

- **Прямое чтение `ticks` без выключения прерываний** — torn read на 32-битной платформе. Исключение — `too_many_loops`: ей важно только «изменилось или нет», а не точное значение.
- **Отсчёт от момента включения прерываний, а не от старта машины.** Всё, что было до `thread_start`, в `ticks` не попало.
- **Потеря тиков.** Если код держит прерывания выключенными дольше тика, PIC запомнит только один ожидающий IRQ0 → `ticks` вырастет на 1 вместо N → часы ОС отстанут навсегда.
- **Константы вместо `TIMER_FREQ`.** `timer_sleep(100)` = «1 секунда» только при `TIMER_FREQ = 100`. Писать `timer_sleep(TIMER_FREQ)`.
- **Точность тика — ±10 мс.** Вызов может прийти в любой момент внутри тика — см. [[pintos-timer-sleep]].

### Безопасность

- **Переполнение не грозит**: `int64_t` при 100 тиках/с переполнится через ~2.9 млрд лет. Но разность `timer_elapsed` и арифметика вида `wake_time - timer_ticks()` могут стать **отрицательными** — `timer_sleep` обязана корректно обрабатывать `ticks <= 0` (тест `alarm-negative`).
- **Знаковое переполнение `int64_t` — UB в C.** Для вычислений из недоверенных источников (например, аргумент будущего syscall `sleep`) сложение `timer_ticks() + ticks` нужно проверять до выполнения.
- **Доступ к портам ввода-вывода (`outb`) — только ядро.** Если бы пользовательский процесс мог писать в порт `0x43`/`0x40` (IOPL в EFLAGS), он перенастроил бы частоту таймера и сломал вытеснение — фактически DoS всей системы.

---

## Связанные темы

[[os-index]] [[interrupts]] [[pintos-preemption]] [[pintos-timer-sleep]] [[pintos-timer-calibrate]]
