# pthreads — многопоточность в Linux

## Что это

Библиотека POSIX для создания потоков внутри одного процесса — несколько параллельных путей выполнения с общей памятью.

## Как работает

Поток vs процесс:
- `fork()` — новый процесс, отдельная память, отдельный PID
- `pthread_create()` — новый поток внутри того же процесса, общая память, один PID

У потоков общее: глобальные переменные, куча, файловые дескрипторы.
У потоков своё: стек вызовов, регистры CPU.

### pthread_create — запустить поток

```c
pthread_t tid;
pthread_create(&tid, NULL, worker, &arg);
//             ^ID   ^атрибуты ^функция ^аргумент
```

`worker` — точка входа потока, его `main`. Сигнатура фиксирована:

```c
void *worker(void *arg) {
    // arg — что передали в pthread_create
    return NULL;
}
```

Сигнатура `void *func(void *arg)` — не переносимость, а **обобщённость**: `pthread_create` одна функция, она не знает какой тип ты передашь. `void *` принимает что угодно — ты сам кастуешь внутри.

### pthread_join — дождаться поток

```c
pthread_join(tid, NULL);
```

Без `pthread_join` — `main` завершится раньше потоков и убьёт их. Аналог `waitpid()` для процессов.

### pthread_mutex_t — защита общей памяти

Проблема: `idx++` — не атомарная операция. CPU раскладывает в три шага: LOAD → ADD → STORE. Два потока могут прочитать одно значение и оба записать одинаковый результат. Это **data race**.

```c
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_lock(&mutex);    // захватить замок — если занят, ждать
results[idx++] = data;         // критическая секция
pthread_mutex_unlock(&mutex);  // отпустить
```

Мьютекс нужен только при записи в общий ресурс. Локальные переменные потока — без замка.

### Передача данных потоку

Через структуру аргумента:

```c
typedef struct {
    int *pids;   // указатель на начало диапазона
    int  count;  // сколько элементов обработать
} ThreadArg;
```

Каждый поток получает свою часть массива:

```c
int chunk = count / 4;
ThreadArg args[4];
args[0] = (ThreadArg){ pid_list,              chunk };
args[1] = (ThreadArg){ pid_list + chunk,       chunk };
args[2] = (ThreadArg){ pid_list + chunk * 2,   chunk };
args[3] = (ThreadArg){ pid_list + chunk * 3,   count - chunk * 3 }; // остаток
```

## Пример

Параллельный сбор данных из /proc:

```c
ProcInfo results[1024];
int results_count = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *worker(void *arg)
{
    ThreadArg *t = (ThreadArg *)arg;
    for (int i = 0; i < t->count; i++) {
        ProcInfo info = {0};                          // локально — без мьютекса
        if (read_proc_status(t->pids[i], &info) == 0) {
            pthread_mutex_lock(&mutex);
            results[results_count++] = info;          // общий массив — с мьютексом
            pthread_mutex_unlock(&mutex);
        }
    }
    return NULL;
}

// в main:
pthread_t tid[4];
for (int i = 0; i < 4; i++)
    pthread_create(&tid[i], NULL, worker, &args[i]);
for (int i = 0; i < 4; i++)
    pthread_join(tid[i], NULL);
```

## Подводные камни

- Забыть `pthread_join` — `main` завершится раньше потоков
- Забыть `closedir` / закрыть ресурсы — утечка
- Мьютекс вокруг слишком большого блока — потоки стоят в очереди, параллелизма нет
- `pid_list` и `results` фиксированного размера — выход за границу если процессов > 1024
- Ядерные потоки (`kworker`, `migration`) не имеют `VmRSS` в `/proc/<pid>/status` — поле останется 0, нужна инициализация `ProcInfo info = {0}`

## Связанные темы

[[fork-linux]] [[proc-filesystem]] [[pipes-linux]]
