---
тема: Вытеснение потоков и Round Robin / Preemption & scheduling (Pintos)
блок: Курс — ОС (Pintos)
дата: 2026-09-14
связано:
  - "[[os-index]]"
  - "[[interrupts]]"
  - "[[pintos-timer-ticks]]"
  - "[[pintos-timer-sleep]]"
  - "[[pthreads-linux]]"
---

# Вытеснение потоков и Round Robin / Preemption & scheduling (Pintos)

## Что это

Вытеснение (preemption) — принудительное отнятие процессора у потока, который исчерпал свой квант времени (time slice). В Pintos это делает прерывание таймера: на каждом тике `timer_interrupt` вызывает `thread_tick()`, та считает, сколько тиков работает текущий поток, и через 4 тика просит переключиться. Выбор следующего потока — алгоритм Round Robin: очередь по кругу.

---

## Ключевые термины (English)

- **preemption** — вытеснение: ОС отбирает процессор без согласия потока
- **time slice / quantum** — квант: сколько поток может работать подряд (`TIME_SLICE = 4` тика = 40 мс)
- **scheduler** — планировщик: решает, какой поток запускать следующим
- **Round Robin** — алгоритм планирования: FIFO-очередь готовых потоков, по кругу
- **ready list / run queue** — очередь потоков, готовых к работе (`ready_list`)
- **context switch** — переключение контекста: сохранить регистры одного потока и загрузить другого
- **idle thread** — поток простоя, работает, когда очередь готовых пуста
- **`hlt`** — инструкция x86: остановить процессор до следующего прерывания
- **cooperative multitasking** — кооперативная многозадачность: потоки отдают процессор сами
- **thread states** — `THREAD_RUNNING`, `THREAD_READY`, `THREAD_BLOCKED`, `THREAD_DYING`
- **deferred yield** — отложенное переключение: флаг в обработчике, `thread_yield` после выхода из него

---

## Как работает

### 1. `thread_tick` — вызывается на каждом тике

```c
/* threads/thread.c:123 */
void
thread_tick (void) 
{
  struct thread *t = thread_current ();

  /* Update statistics. */
  if (t == idle_thread)
    idle_ticks++;
#ifdef USERPROG
  else if (t->pagedir != NULL)
    user_ticks++;
#endif
  else
    kernel_ticks++;

  /* Enforce preemption. */
  if (++thread_ticks >= TIME_SLICE)
    intr_yield_on_return ();
}
```

`thread_current()` возвращает поток, который **был прерван** этим тиком: обработчик выполняется на его стеке, поэтому «текущий» — он.

### 2. Статистика

```c
/* threads/thread.c:49 */
static long long idle_ticks;    /* # of timer ticks spent idle. */
static long long kernel_ticks;  /* # of timer ticks in kernel threads. */
static long long user_ticks;    /* # of timer ticks in user programs. */
```

Каждый тик попадает ровно в один счётчик — в зависимости от того, кого прервали: idle-поток, пользовательский процесс (есть своя таблица страниц `pagedir`, только при `USERPROG`) или поток ядра. Это выборочная оценка: считается, что весь интервал 10 мс занимал тот, кого застал тик.

Печатается при выключении (`devices/shutdown.c:126`):

```
Timer: 312 ticks
Thread: 250 idle ticks, 62 kernel ticks, 0 user ticks
```

Сумма второй строки = первой строке.

### 3. Квант: `TIME_SLICE` и `thread_ticks`

```c
/* threads/thread.c:54 */
#define TIME_SLICE 4            /* # of timer ticks to give each thread. */
static unsigned thread_ticks;   /* # of timer ticks since last yield. */
```

- `thread_ticks` — сколько тиков работает **текущий** поток. Одна глобальная переменная, не поле `struct thread`: процессор один, работающий поток один.
- Сбрасывается в 0 при **любом** переключении — в `thread_schedule_tail` (`threads/thread.c:530`). Новый поток всегда начинает с полного кванта, неважно, отдал ли предыдущий процессор сам или его вытеснили.

### 4. Почему не `thread_yield()` прямо из обработчика

```c
/* threads/thread.c:316 */
ASSERT (!intr_context ());
```

Внутри обработчика внешнего прерывания `intr_context()` = true — ядро упадёт. Обработка ещё не закончена: PIC не получил EOI, `in_external_intr` выставлен. Поэтому обработчик только ставит флаг:

```c
/* threads/interrupt.c:222 */
void
intr_yield_on_return (void) 
{
  ASSERT (intr_context ());
  yield_on_return = true;
}
```

а `intr_handler` проверяет его **после** обработчика:

```c
/* threads/interrupt.c:378 */
if (external) 
  {
    in_external_intr = false;             // 1. больше не «внутри» обработчика
    pic_end_of_interrupt (frame->vec_no); // 2. PIC может присылать следующие
    if (yield_on_return)                  // 3. только теперь переключаемся
      thread_yield (); 
  }
```

### 5. `thread_yield` → `schedule` → Round Robin

```c
/* threads/thread.c:311 */
void
thread_yield (void) 
{
  struct thread *cur = thread_current ();
  enum intr_level old_level;
  
  ASSERT (!intr_context ());

  old_level = intr_disable ();
  if (cur != idle_thread) 
    list_push_back (&ready_list, &cur->elem);   // в КОНЕЦ очереди
  cur->status = THREAD_READY;
  schedule ();
  intr_set_level (old_level);
}

/* threads/thread.c:557 */
static void
schedule (void) 
{
  struct thread *cur = running_thread ();
  struct thread *next = next_thread_to_run ();
  struct thread *prev = NULL;

  ASSERT (intr_get_level () == INTR_OFF);
  ASSERT (cur->status != THREAD_RUNNING);
  ASSERT (is_thread (next));

  if (cur != next)
    prev = switch_threads (cur, next);   // threads/switch.S
  thread_schedule_tail (prev);           // status = RUNNING, thread_ticks = 0
}

/* threads/thread.c:495 */
static struct thread *
next_thread_to_run (void) 
{
  if (list_empty (&ready_list))
    return idle_thread;
  else
    return list_entry (list_pop_front (&ready_list), struct thread, elem);  // из НАЧАЛА
}
```

Кладём в конец, берём из начала — FIFO, потоки ходят по кругу. **Приоритет `t->priority` сейчас не учитывается** — это задание Priority Scheduling проекта 1.

`switch_threads` сохраняет регистры и `esp` текущего потока в его `struct thread`, загружает `esp` следующего и возвращается уже **на его стеке**.

### 6. Хронология: два потока A и B крутят `while(1)`

```
тик:        1     2     3     4     5     6     7     8     9
            │     │     │     │     │     │     │     │     │
работает:   A  A  A  A  A  A  A  A│ B  B  B  B  B  B  B  B│ A ...
thread_ticks:1    2     3     4   │ 1     2     3     4   │ 1
                              │   │                   │   │
                    4 >= TIME_SLICE                4 >= TIME_SLICE
                    yield_on_return = true         yield_on_return = true
                    thread_yield                   thread_yield
                    ready_list: [B] → [A]          ready_list: [A] → [B]
```

`ticks` при этом растёт непрерывно — это общие часы.

### 7. Куда «девается» прерванный поток

Переключение происходит **изнутри обработчика прерывания** потока A. В момент `switch_threads` его стек:

```
стек потока A (сверху — самое свежее):
  ┌─────────────────────────────┐
  │ switch_threads  (сохранено) │  ← A «заморожен» здесь
  │ schedule                    │
  │ thread_yield                │
  │ intr_handler                │
  │ struct intr_frame           │  ← регистры A на момент тика
  │ EFLAGS, CS, EIP             │  ← положил процессор
  │ ...код A: while(1)...       │
  └─────────────────────────────┘
```

Когда планировщик снова выберет A, цепочка раскрутится обратно: `switch_threads` → `schedule` → `thread_yield` → `intr_handler` → `intr_exit` → `iret` вернёт `EIP` и `EFLAGS` с IF = 1. Поток продолжит `while(1)` с той же инструкции и не узнает, что стоял 40 мс.

Во время переключения прерывания выключены. Новый поток B восстанавливает **свой** IF: если B был вытеснен — через `iret`, если отдал процессор сам — через `intr_set_level(old_level)` в конце `thread_yield`.

### 8. Idle-поток

```c
/* threads/thread.c:398 */
for (;;) 
  {
    intr_disable ();
    thread_block ();                            // отдать процессор, если кто-то появился
    asm volatile ("sti; hlt" : : : "memory");   // включить прерывания и «уснуть»
  }
```

- Запускается, когда `ready_list` пуст.
- `hlt` останавливает процессор до следующего прерывания — настоящий простой, без холостого цикла. Разбудит его в худшем случае таймер через ≤10 мс, тик попадёт в `idle_ticks`.
- `sti; hlt` — вместе не случайно: после `sti` прерывания разрешаются только **после следующей инструкции**. Если бы прерывание пришло между `sti` и `hlt`, его обработали бы до засыпания, и `hlt` проспал бы лишний тик.

---

## Итоговая схема

```
тик 8254
  └─ timer_interrupt
       ├─ ticks++                          ← часы ОС
       └─ thread_tick
            ├─ idle/kernel/user_ticks++    ← статистика
            └─ ++thread_ticks >= 4 ?
                 └─ intr_yield_on_return   ← только флаг
  └─ intr_handler (после обработчика и EOI)
       └─ yield_on_return ? thread_yield
            └─ в конец ready_list → schedule → switch_threads → thread_ticks = 0
```

В проекте 1 сюда же добавятся расчёты MLFQS (`thread_mlfqs`): раз в секунду (`ticks % TIMER_FREQ == 0`) — `load_avg` и `recent_cpu`, раз в 4 тика — приоритеты.

---

## Подводные камни

- **`thread_yield` из обработчика** → `ASSERT (!intr_context ())`. Только `intr_yield_on_return`.
- **Долгий `thread_tick`.** Выполняется 100 раз в секунду с выключенными прерываниями. Тяжёлые вычисления (проход по всем потокам на каждом тике) заметно замедляют систему и искажают тайминги тестов.
- **`thread_ticks` — не счётчик конкретного потока.** Хранить «сколько работал поток» для MLFQS нужно в `struct thread` (`recent_cpu`), а не здесь.
- **Idle-поток не кладётся в `ready_list`** — проверка `cur != idle_thread` в `thread_yield`. Если бы клали, очередь никогда не была бы пустой.
- **Нельзя `printf` между `switch_threads` и концом `thread_schedule_tail`** — переключение не завершено (комментарий `threads/thread.c:513`).
- **Без таймера вытеснения нет.** Если прерывания выключены навсегда (забытый `intr_set_level`), планировщик становится кооперативным: зациклившийся поток вешает систему.

### Безопасность

- **Вытеснение — защита от DoS.** Именно прерывание таймера не даёт пользовательскому процессу с `while(1)` захватить процессор. Всё, что позволяет процессу выключить прерывания (IOPL, `cli` в ring 3), ломает эту гарантию.
- **Порча `ready_list`.** Список меняется только при выключенных прерываниях. Если модификация списка может быть прервана тиком, который тоже зовёт `thread_yield`, двусвязный список рвётся: потерянный поток, цикл в списке или обращение к освобождённому `struct thread` (use-after-free — `palloc_free_page(prev)` в `thread_schedule_tail`).
- **Переполнение стека ядра.** `struct thread` и стек потока делят одну страницу 4 КБ; стек растёт вниз к структуре. Цепочка обработчиков + большие локальные массивы перетирают поле `magic` → `is_thread()` падает в `ASSERT`. В ядре не выделять большие буферы на стеке.

---

## Связанные темы

[[os-index]] [[interrupts]] [[pintos-timer-ticks]] [[pintos-timer-sleep]] [[pthreads-linux]]
