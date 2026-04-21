# Сигналы Linux

## Что это
Механизм межпроцессного взаимодействия: ядро или другой процесс отправляет процессу асинхронное уведомление, которое прерывает нормальное выполнение.

## Как работает

1. Кто-то отправляет сигнал: `kill(pid, SIGTERM)` или нажатие Ctrl+C
2. Ядро запоминает сигнал в таблице процесса
3. При следующем удобном моменте (обычно возврат из syscall) — ядро прерывает процесс
4. Вызывается зарегистрированный обработчик (`handler`)
5. После обработчика процесс продолжает с того места где прервался (или завершается)

## Ключевые сигналы

| Сигнал | Номер | Смысл | Перехватить |
|--------|-------|-------|-------------|
| SIGINT | 2 | Ctrl+C — прервать | да |
| SIGTERM | 15 | Попросить завершиться | да |
| SIGKILL | 9 | Убить немедленно | нет — обрабатывает ядро |

`SIGKILL` нельзя перехватить намеренно — иначе процесс мог бы заблокировать завершение.

## Пример

```c
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static FILE *log_file;

void handler(int sig)
{
    if (sig == SIGINT) {
        fprintf(log_file, "[%u] caught SIGINT\n", getpid());
        return;
    } else if (sig == SIGTERM) {
        fprintf(log_file, "[%u] caught SIGTERM, shutting down\n", getpid());
        exit(0);
    }
}

int main(void)
{
    log_file = fopen("signals.log", "w");
    fprintf(log_file, "[%u] started\n", getpid());

    struct sigaction sa;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);   // регистрировать ДО цикла
    sigaction(SIGTERM, &sa, NULL);

    while (1) { pause(); }
    return 0;
}
```

## Подводные камни

- `sigaction()` регистрировать **до** цикла — иначе первый сигнал придёт без обработчика
- `fprintf` **не async-signal-safe** — вызов из handler технически UB (может испортить буферы stdio). Безопасная альтернатива — `write()` напрямую.
- Забыть `getpid()` в `fprintf` с `%u` — читает мусор со стека вместо PID
- `\n` в конце `fprintf` обязателен — без него строки сливаются в файле
- `signal()` вместо `sigaction()` — поведение непредсказуемо на разных платформах

## signal() vs sigaction()

`signal()` проще, но после срабатывания может сброситься в default. `sigaction()` стабилен — обработчик остаётся пока не заменишь явно.

## Связанные темы

[[fork-linux]] [[proc-filesystem]]
