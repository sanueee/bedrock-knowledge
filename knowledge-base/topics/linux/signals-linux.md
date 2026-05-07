# Сигналы Linux / Signals

## Что это

Механизм межпроцессного взаимодействия: ядро или другой процесс отправляет процессу асинхронное уведомление, которое прерывает нормальное выполнение. Семантически — программный аналог hardware interrupt: текущий поток выполнения замораживается между инструкциями, управление передаётся в зарегистрированный обработчик, после возврата из обработчика — продолжение с того же места.

---

## Ключевые термины (English)

- **signal handler** — функция-обработчик сигнала, регистрируется через `signal()` или `sigaction()`
- **async-signal-safe** — свойство функции: можно безопасно вызывать из signal handler
- **pending signal** — сигнал отправлен, но handler ещё не вызван (бит в task_struct)
- **signal mask** — набор сигналов, заблокированных для текущего потока (отложат выполнение, не потеряют)
- **signal disposition** — что произойдёт по умолчанию: terminate / ignore / core dump / stop / continue
- **interrupted system call** — syscall прерванный сигналом, возвращает `-1` с `errno = EINTR`
- **reentrant function** — функция безопасная для повторного входа (включая прерывание сигналом)
- **race condition in handler** — гонка между handler и main кодом за разделяемые данные

---

## Как работает (модель)

1. Кто-то отправляет сигнал: `kill(pid, SIGINT)` или нажатие Ctrl+C (терминал шлёт SIGINT процессу в foreground process group).
2. Ядро ставит **pending bit** в `task_struct` процесса. Это бит, не очередь — повторные сигналы того же типа схлопываются в один (real-time signals SIGRTMIN..SIGRTMAX — исключение, они queueable).
3. Перед следующим возвратом в user space (после syscall, после обработки interrupt, на вытеснении) — ядро проверяет pending. Если есть и сигнал не заблокирован маской — ядро вызывает handler.
4. Handler выполняется **в том же потоке, на том же стеке** что и main. Это **не отдельный поток**. Регистры (PC, SP, ...) пушатся на стек, после возврата handler'а — восстанавливаются.
5. На время выполнения handler'а тот же сигнал по умолчанию заблокирован (нельзя прервать handler сигналом того же типа). Это поведение `sigaction`. Через `sa_mask` можно дополнительно блокировать другие.
6. Если был блокирующий syscall (`read`, `accept`) — он **прервётся**. Поведение после возврата handler'а зависит от `SA_RESTART` (см. ниже).

---

## Ключевые сигналы

| Сигнал | Номер | Default disposition | Перехватить | Откуда обычно |
|--------|-------|---------------------|-------------|---------------|
| SIGINT | 2 | Term | да | Ctrl+C |
| SIGTERM | 15 | Term | да | `kill <pid>`, `systemctl stop` |
| SIGKILL | 9 | Term | **нет** | `kill -9` — гарантированное завершение |
| SIGSTOP | 19 | Stop | **нет** | `Ctrl+Z` (точнее SIGTSTP), debugger |
| SIGCONT | 18 | Cont | да | `fg`/`bg`, ядро при ресурсе |
| SIGSEGV | 11 | Term + Core | да (но обычно не надо) | invalid memory access |
| SIGCHLD | 17 | Ign | да | дочерний процесс завершился |
| SIGPIPE | 13 | Term | да | `write()` в закрытый pipe/socket |
| SIGHUP | 1 | Term | да | терминал отвалился; в демонах — "перечитай конфиг" |
| SIGUSR1/USR2 | 10/12 | Term | да | пользовательские, для приложения |

**SIGKILL и SIGSTOP нельзя перехватить, заблокировать или проигнорировать** — это гарантия ядра. Без неё ни `kill -9`, ни остановка процесса при нехватке ресурсов не работали бы надёжно.

---

## signal() vs sigaction()

### `signal()` — старое API (POSIX.1-1990)

```c
void (*signal(int signum, void (*handler)(int)))(int);
```
Принимает **указатель на функцию**. Историческая проблема: на System V после первого срабатывания handler **сбрасывался на default** — нужно было заново вызывать `signal()` внутри handler'а, что давало классическую гонку (между сбросом и переустановкой может прилететь второй сигнал → дефолтное действие → обычно kill процесса).

На BSD и в современном glibc — handler **не сбрасывается**, ведёт себя как `sigaction()`. Но **это зависит от libc и флагов компиляции** — поведение неоднозначное. В production не использовать.

### `sigaction()` — новое API (POSIX.1-2001)

```c
int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);

struct sigaction {
    void (*sa_handler)(int);    // простой handler
    void (*sa_sigaction)(int, siginfo_t *, void *);  // расширенный (с SA_SIGINFO)
    sigset_t sa_mask;            // какие сигналы блокировать на время handler'а
    int sa_flags;                // флаги поведения
    // sa_restorer — implementation-specific, не трогать
};
```

**Принципиально отличается**: handler **не сбрасывается** (поведение детерминированное), плюс есть флаги (`SA_RESTART`, `SA_SIGINFO`, `SA_NODEFER` и др.) для тонкой настройки.

### Сравнение

| | `signal()` | `sigaction()` |
|---|---|---|
| Сброс handler'а | зависит от libc | никогда |
| Доп. информация (PID отправителя и т.д.) | нет | да через `siginfo_t` |
| Контроль перезапуска syscall | нет | через `SA_RESTART` |
| Контроль маски на время handler'а | нет | через `sa_mask` |
| Современное использование | **не использовать** | стандарт |

**Правило**: в production-коде только `sigaction()`. `signal()` встречается только в учебниках и в legacy.

---

## Канонический паттерн установки

```c
struct sigaction sa = {0};                      // обнулить ВСЕ поля (важно)
sa.sa_handler = handler;
sigemptyset(&sa.sa_mask);                       // не блокировать дополнительные сигналы
sa.sa_flags = SA_RESTART;                       // авто-перезапуск syscall
if (sigaction(SIGINT, &sa, NULL) == -1) {       // ВСЕГДА проверять возврат
    perror("sigaction");
    exit(1);
}
```

Почему `= {0}`: в `struct sigaction` могут быть поля implementation-specific (`sa_restorer` на Linux). Локальная переменная на стеке без инициализации содержит мусор — ядро может прочитать невалидный указатель.

---

## SA_RESTART — поведение прерванных syscalls

Без `SA_RESTART`: блокирующий syscall, прерванный сигналом, возвращает `-1` с `errno = EINTR`. Программа должна сама перезапустить:

```c
ssize_t n;
while ((n = read(fd, buf, sizeof(buf))) == -1 && errno == EINTR) {
    /* перезапустить */
}
```

С `SA_RESTART`: ядро автоматически перезапускает большинство syscalls после возврата handler'а. Цикл выше становится не нужен.

**Не все syscalls перезапускаются** даже с `SA_RESTART` — список зависит от версии ядра и POSIX. Например `sleep()`, `pause()`, `select()`, `poll()` — обычно **не** перезапускаются (`SA_RESTART` на них не действует). Они для того и нужны — завершиться по сигналу.

В простых учебных программах с `pause()` флаг `SA_RESTART` практически не виден. В сетевых программах и I/O-кодах — критичен.

---

## Async-signal-safety — главное ограничение handler'а

Handler может сработать **в любой момент между инструкциями**, в том числе пока main код находится в середине вызова библиотечной функции. Если handler вызовет ту же функцию — могут произойти:

1. **Деadlock** на внутреннем мьютексе. Например main вызвал `printf("hello\n")` → libc взял внутренний lock на stdio → handler вызвал `printf("got SIGINT\n")` → попытался взять тот же lock → процесс замер навсегда.
2. **Порча состояния**. Например `malloc()` использует внутренние списки free-блоков. Прерывание в середине обновления списка → handler вызывает `malloc()` → видит частично обновлённое состояние → corruption.
3. **Реентерабельность**. Функции хранящие state в static переменных (`strtok()`, `gmtime()`, `gethostbyname()`) дают мусор при повторном входе.

**Async-signal-safe** = функция, которую гарантированно безопасно вызывать из handler'а. POSIX определяет точный список — `man 7 signal-safety`. Ключевые из него:

| Категория | Безопасные | Небезопасные |
|---|---|---|
| Низкоуровневое I/O | `write`, `read`, `open`, `close` | `printf`, `fprintf`, `fopen`, `fwrite` |
| Память | — | `malloc`, `free`, `realloc` |
| Время | `time`, `clock_gettime` | `localtime`, `gmtime` (используют static) |
| Процессы | `kill`, `_exit`, `fork`, `getpid` | `exit` (вызывает atexit, может быть не safe) |
| Сигналы | `signal`, `sigaction`, `sigprocmask` | — |
| Строки | — | `strerror`, `strtok` (static state) |

**Базовое правило**: в handler'е делай **только syscalls** (через `write`, `_exit` и т.д.), и устанавливай флаги. Всё что использует stdio, malloc, time-форматирование — **в main loop**, не в handler.

---

## Канонический паттерн "флаг + main loop"

Идиома для общения handler ↔ main:

```c
#include <signal.h>

static volatile sig_atomic_t signal_count = 0;

void handler(int sig) {
    signal_count++;                             // не printf!
    const char msg[] = "got signal\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1); // write — async-signal-safe
}

int main(void) {
    /* установка handler через sigaction */

    sig_atomic_t last_seen = 0;
    while (1) {
        pause();                                // спим до сигнала
        while (last_seen < signal_count) {
            last_seen++;
            printf("count=%d\n", last_seen);    // printf здесь — в main, безопасно
        }
    }
}
```

Каждый модификатор переменной решает свою проблему:

- **`sig_atomic_t`** — typedef из `<signal.h>`, обычно равен `int`. Гарантирует что чтение или запись происходит за **одну машинную инструкцию** — handler не может прервать запись посередине и прочитать "разорванное" значение (torn read). **Не** гарантирует атомарность read-modify-write вроде `count++` — это всё равно три инструкции (load, inc, store).
- **`volatile`** — запрет компилятору кешировать значение в регистре. Без него цикл `while (last_seen < signal_count)` мог быть оптимизирован в `if (last_seen < signal_count) while(1) {}` — компилятор не знает, что значение меняет handler. С `volatile` — каждое чтение реально из памяти.
- **`static`** на глобальной переменной — internal linkage, переменная видна только в этом `.c`-файле. Защита от коллизий имён в больших проектах.

`volatile` ≠ atomic, не делает код thread-safe. Это **только про оптимизатор**, не про конкурентность. Для multi-threaded — atomics из `<stdatomic.h>` или мьютексы.

---

## Что наследуется при fork/exec

| Действие | Сигнальные обработчики |
|---|---|
| `fork()` | child наследует **все** установленные handler'ы родителя |
| `exec()` | custom handler'ы **сбрасываются на default**; ignored сигналы остаются ignored |
| `pthread_create` | потоки разделяют общие handler'ы (handler один на процесс) |

Поэтому новая программа после `exec()` сама ставит свои handler'ы — старые из родительского процесса не наследуются. Это и логично: handler — это указатель на функцию в адресном пространстве, а после exec адресное пространство полностью заменяется.

---

## Подводные камни

- **`printf`/`fprintf`/`malloc` в handler'е** — UB. Может произойти deadlock или corruption. Использовать `write()` напрямую.
- **`signal()` без `\n`** в стандартном выводе из handler — буферизация stdio плюс не-async-signal-safe == проблема в квадрате. `write()` идёт мимо буферов libc, проблема снимается.
- **Не проверять возврат** `signal()` (`SIG_ERR`) или `sigaction()` (`-1`) — стандартная ошибка, прячет невалидную установку (например попытку перехватить SIGKILL).
- **`struct sigaction sa;`** без инициализации — мусор в полях. Всегда `= {0}` или `memset`.
- **`signal()` в новом коде** — поведение зависит от libc, нельзя на это полагаться.
- **Pending signals не считаются** — три SIGINT за секунду пока handler работает = одно повторное срабатывание после возврата, не три. Если нужно подсчитать события точно — счётчик инкрементируется в handler'е.
- **`sleep()`, `pause()`, `select()` не перезапускаются с `SA_RESTART`** — они и так возвращаются по сигналу, это и есть их назначение.

---

## Заголовки

```c
#include <signal.h>      // sigaction, sigemptyset, sig_atomic_t, SIGINT и т.д.
#include <unistd.h>      // pause, write, STDOUT_FILENO
```

---

## Man pages

- `man 2 sigaction`
- `man 2 signal`
- `man 7 signal` — общий обзор сигналов
- `man 7 signal-safety` — полный список async-signal-safe функций

## Связанные темы

[[fork-linux]] [[pipes-linux]] [[proc-filesystem]]
