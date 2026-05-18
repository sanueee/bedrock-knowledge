---
тема: Async-signal-safe код / Async-signal-safety
блок: Фундамент (theory)
дата: 2026-05-14
связано:
  - "[[signals-linux]]"
  - "[[fork-linux]]"
  - "[[pthreads-linux]]"
  - "[[virtual-memory-cow]]"
  - "[[syscalls-linux]]"
---

# Async-signal-safe код / Async-signal-safety

## Что это

Свойство функции: её можно **безопасно вызывать из signal handler**. Это **не то же самое** что thread-safe. Async-signal-safety — более жёсткое требование, потому что signal handler выполняется **в том же треде** что прерванный код, в произвольной точке.

## Ключевые термины (English)

- **signal handler** — функция, которую ядро вызывает при доставке сигнала
- **async-signal-safe** — безопасная для вызова из signal handler
- **reentrant** — функция которую можно прервать и заново вызвать без порчи состояния
- **self-deadlock** — самоблокировка одного треда из-за вложенного захвата того же mutex
- **atfork handler** — callback зарегистрированный через `pthread_atfork`, запускается при `fork()`

## Почему signal handler — особая среда

Signal handler **не** выполняется параллельно — он **перехватывает** текущий тред. CPU посередине инструкции (точнее: между двумя инструкциями) получает сигнал, ядро сохраняет регистры, вызывает handler. Когда handler возвращается — регистры восстанавливаются, прерванный код продолжается.

Следствия:
- Прерванный код мог держать mutex — handler в том же треде не может его взять, **self-deadlock**.
- Прерванный код мог быть в середине `malloc` — heap в неконсистентном состоянии (часть структур обновлена, часть нет). Handler позовёт `malloc` → коррапт heap.
- Прерванный код мог быть в середине обновления глобальной переменной (`errno`!) — handler перезатрёт.

Это **не параллельность**. Это **вложенный вызов в произвольной точке**.

## POSIX-список безопасных функций

POSIX определяет точный список async-signal-safe функций (man 7 signal-safety). Примерный список:

**Можно:**
- `_exit`, `abort`
- `read`, `write`, `open`, `close`, `dup`, `pipe`
- `kill`, `signal`, `sigaction`, `sigprocmask`
- `fork` (но не любой fork — см. ниже), `_Fork`, `execve`
- `getpid`, `getppid`, `getuid`

**Нельзя:**
- `printf`, `fprintf`, `sprintf` (используют буферы stdio + locale + heap)
- `malloc`, `free`, `realloc` (нерентрабельны)
- `pthread_mutex_lock` (self-deadlock)
- `exit` (запустит atexit-handlers — пользовательский код)
- `fork` от **glibc** (см. ниже)

Общее правило: тонкие обёртки над сисколлами — safe. Всё что трогает глобальное состояние библиотеки (буферы, locale, heap, atfork) — unsafe.

## fork vs _Fork — главный разрыв

**`fork()` (POSIX)** запускает зарегистрированные через `pthread_atfork` handlers — пользовательский код, который POSIX **не** обязывает быть async-signal-safe. Тип atfork-callback'ов:

```c
pthread_atfork(prepare, parent, child);
```

`prepare` обычно берёт mutex'ы библиотек чтобы зафиксировать состояние. `parent`/`child` их отпускают. Если такой callback написан с `malloc` или другим unsafe-кодом → `fork()` сам по себе становится unsafe из signal handler.

**`_Fork()` (C11/POSIX 2024)** — это `fork` **без atfork-handlers**. Только сисколл `clone` + минимум работы. Async-signal-safe by design.

Поэтому musl в `posix_spawn` зовёт `_Fork`, не `fork`:
- POSIX декларирует `posix_spawn` как async-signal-safe.
- Если бы он звал обычный `fork` — нарушил бы декларацию (любой пользовательский atfork-callback мог бы быть unsafe).
- См. [[reading-01]] и [musl-popen.md](../reading/musl-popen.md).

## Self-deadlock — типичный сценарий

```c
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

void handler(int sig) {
    pthread_mutex_lock(&m);     // ← вторая попытка взять тот же mutex в том же треде
    // ...
}

int main() {
    signal(SIGINT, handler);
    pthread_mutex_lock(&m);     // взяли mutex
    // ← сюда приходит SIGINT, тред уходит в handler
    // → handler пытается взять m, тред засыпает на mutex
    // → разбудить его некому, mutex держит этот же тред
    // → self-deadlock
}
```

Это **один тред, две роли** — узнаваемый паттерн (тот же класс что fclose→wait4 в [[reading-01]] выводе 2).

## Связь со специфическими доменами

Этот вопрос всплывает не только в signal handlers. То же ограничение работает в:

- **Interrupt context в ядре** — нельзя спать, нельзя брать sleep-mutex (только spinlock), нельзя аллоцировать с `GFP_KERNEL` (только `GFP_ATOMIC`). Это та же дисциплина.
- **eBPF-программы** — выполняются в kprobe/tracepoint контексте, разрешены только bpf helper'ы (аналог POSIX safe-list).
- **NMI handlers** в ядре — ещё более жёсткие правила, потому что NMI могут прервать даже interrupt handler.
- **Audit/EDR-агенты** (профильная область) — часто хукаются в системные пути и работают в контекстах с похожими ограничениями.

## Подводные камни

- **`errno` глобален** (хоть и thread-local в современных libc). Handler может его перезатереть → после возврата прерванный код увидит чужой errno. Дисциплина: сохранять `errno` в начале handler'а, восстанавливать в конце.
- **stdio (`printf`)** в handler — частая ошибка, "вроде работает". На самом деле undefined behavior: locale-кэш, FILE-буферы, mutex stdout. На малых нагрузках не падает — на больших коррапт.
- **Async-cancel-safety ≠ async-signal-safety** — отдельная категория про `pthread_cancel`. Близко, но не одно и то же.
- **`signal()` vs `sigaction()`** — `signal()` на разных системах ведёт себя по-разному (resets handler? mask other signals?). Для предсказуемого поведения — всегда `sigaction()`. См. [[signals-linux]].

## Связанные темы

[[signals-linux]] [[pthreads-linux]] [[fork-linux]] [[virtual-memory-cow]] [[reading-01]]
