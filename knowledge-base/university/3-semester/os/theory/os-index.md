---
тема: Курс ОС (Pintos) — карта конспектов / OS course index
блок: Курс — ОС (Pintos)
дата: 2026-09-14
связано:
  - "[[interrupts]]"
  - "[[pintos-timer-ticks]]"
  - "[[pintos-preemption]]"
  - "[[pintos-timer-sleep]]"
  - "[[pintos-timer-calibrate]]"
---

# Курс ОС (Pintos) — карта конспектов / OS course index

## Что это

Хаб конспектов по курсу операционных систем на учебной ОС Pintos (x86, 32 бита). Исходники — `pintos/src/`: `threads/` (потоки, прерывания, планировщик), `devices/` (таймер, диск, клавиатура), `userprog/` (процессы, syscalls), `tests/`.

---

## Ключевые термины (English)

- **Pintos** — учебная ОС Стэнфорда для x86, запускается в эмуляторе (QEMU/Bochs)
- **kernel thread** — поток ядра
- **tick** — период прерывания таймера (10 мс)
- **interrupt** — сигнал процессору прервать текущий код и выполнить обработчик
- **scheduler** — планировщик потоков
- **busy waiting** — активное ожидание в цикле

---

## Порядок чтения: таймер и планирование

1. [[interrupts]] — что такое прерывания, IDT, PIC, виды прерываний, `cli`/`sti`, контекст прерывания.
2. [[pintos-timer-ticks]] — микросхема 8254, как появляется тик, путь до `ticks++`, `timer_ticks()`.
3. [[pintos-preemption]] — `thread_tick`, квант `TIME_SLICE`, отложенный `thread_yield`, Round Robin, idle-поток.
4. [[pintos-timer-sleep]] — API таймера, почему `timer_sleep` через `thread_yield` плохой, идея Alarm Clock, тесты `alarm-*`.
5. [[pintos-timer-calibrate]] — `timer_calibrate`, `loops_per_tick`, `barrier()`, формула `real_time_delay`, точность.

---

## Общая картина

```
            ЖЕЛЕЗО                              ПРОГРАММА
┌────────────────────────┐
│ 8254, канал 0          │  pit_configure_channel(0, 2, 100)
│ 1193180 Гц / 11932     │◄─────────────── timer_init
│ → импульс каждые 10 мс │
└───────────┬────────────┘
            │ IRQ0
┌───────────▼────────────┐
│ PIC 8259 → вектор 0x20 │  pic_init: IR0 → 0x20
└───────────┬────────────┘
            │ (если IF = 1)
            ▼
   intr20_stub → intr_entry → intr_handler                     (interrupts)
            │
            ▼
   timer_interrupt ──────────────┬──────────────────────────────┐
     ticks++                     │                              │
            │               thread_tick                  (Alarm Clock:
            │                 idle/kernel/user_ticks++    разбудить спящих)
            │                 ++thread_ticks >= 4 ?
            │                   → intr_yield_on_return
            │                              │
            │         intr_handler: EOI, thread_yield → Round Robin
            │                                                  (pintos-preemption)
            ├── timer_ticks / timer_elapsed ── тесты, MLFQS    (pintos-timer-ticks)
            ├── timer_sleep (≥ 1 тика) ◄── msleep/usleep/nsleep (pintos-timer-sleep)
            │
            └── эталон для калибровки (один раз при загрузке)
                         │
                         ▼
                  loops_per_tick                               (pintos-timer-calibrate)
                         │
                         ▼
               real_time_delay → busy_wait
                  ▲                ▲
   msleep/usleep/nsleep     mdelay / udelay / ndelay
   (если < 1 тика)          (всегда; можно без прерываний)
```

---

## Шпаргалка: счётчики времени

| Сущность | Что это | Кто меняет | Когда | Кто использует |
|---|---|---|---|---|
| `ticks` | часы ОС, число прерываний таймера | `timer_interrupt` | каждые 10 мс | `timer_ticks`, `timer_sleep`, калибровка, тесты |
| `thread_ticks` | сколько тиков работает текущий поток | `thread_tick` (+1), `thread_schedule_tail` (= 0) | каждый тик / каждое переключение | вытеснение по `TIME_SLICE` |
| `idle/kernel/user_ticks` | статистика | `thread_tick` | каждый тик | `thread_print_stats` при выключении |
| `loops_per_tick` | итераций `busy_wait` за один тик | `timer_calibrate` | один раз при загрузке | `real_time_delay` |

## Шпаргалка: порядок инициализации (`threads/init.c`)

```c
thread_init ();      // :90   текущий код становится потоком
intr_init ();        // :109  IDT, переназначение PIC
timer_init ();       // :110  PIT 100 Гц, обработчик на 0x20
kbd_init ();
input_init ();
thread_start ();     // :119  idle-поток + intr_enable() → ticks пошёл
serial_init_queue ();
timer_calibrate ();  // :121  loops_per_tick
```

---

## Подводные камни (сквозные)

- Всё, что общее у обработчика прерывания и обычного кода, — только при выключенных прерываниях, не под локом.
- В обработчике: не спать, не `thread_yield`, работать быстро.
- `intr_set_level(old_level)`, а не `intr_enable()`.
- Прерывания, выключенные дольше тика, теряют тики.

---

## Связанные темы

[[interrupts]] [[pintos-timer-ticks]] [[pintos-preemption]] [[pintos-timer-sleep]] [[pintos-timer-calibrate]] [[signals-linux]] [[syscalls-linux]]
