---
тема: Сортировка в C — qsort / Sorting
блок: A — Linux syscalls
дата: 2026-04-13
связано:
  - "[[directory-traversal-c]]"
  - "[[proc-filesystem]]"
---

# Сортировка в C — qsort

## Суть (своими словами)

В C нет встроенной сортировки как метода. Есть `qsort` из `<stdlib.h>` — универсальная функция быстрой сортировки. Работает с любым массивом через указатель `void*` и функцию-компаратор.

## Сигнатура qsort

```c
void qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *));
```

- `base` — указатель на начало массива
- `nmemb` — количество элементов
- `size` — размер одного элемента в байтах (`sizeof`)
- `compar` — функция сравнения

## Компаратор — правила

```c
int cmp(const void *a, const void *b);
```

Должен вернуть:
- **отрицательное** — `a` идёт раньше `b`
- **положительное** — `b` идёт раньше `a`
- **0** — равны

**По возрастанию:** `a - b` (для целых чисел)
**По убыванию:** `b - a`

Но вычитание опасно при больших числах (переполнение). Безопаснее явное сравнение.

## Пример из моего кода — сортировка процессов по VmRSS по убыванию

```c
typedef struct {
    char     pid[21];
    uint64_t vmrss_kb;
    char     name[64];
} ProcInfo;

int cmp_proc(const void *a, const void *b)
{
    const ProcInfo *pa = (const ProcInfo *)a;
    const ProcInfo *pb = (const ProcInfo *)b;
    if (pa->vmrss_kb == pb->vmrss_kb) return 0;
    return (pa->vmrss_kb > pb->vmrss_kb) ? -1 : 1;
}

// Использование:
qsort(proc_arr, count, sizeof(ProcInfo), cmp_proc);
```

## Частая ошибка — неверная сигнатура компаратора

```c
// Неправильно — компилятор выдаст warning/error:
int cmp_proc(ProcInfo *a, ProcInfo *b) { ... }

// Правильно — строго const void*:
int cmp_proc(const void *a, const void *b) { ... }
```

`qsort` ожидает `const void *` — приведение типа делается внутри компаратора вручную.

## Почему нельзя вычитать uint64_t в компараторе

```c
// Опасно — при a=0 и b=1 результат обернётся в огромное число:
return pa->vmrss_kb - pb->vmrss_kb;

// Безопасно — явное сравнение:
return (pa->vmrss_kb > pb->vmrss_kb) ? -1 : 1;
```

## Man pages / docs

- `man 3 qsort`

## Связанные темы

[[directory-traversal-c]], [[proc-filesystem]]
