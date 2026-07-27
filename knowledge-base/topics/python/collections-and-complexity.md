---
тема: Python-обёртки над структурами данных / collections + сложность операций
блок: Z — Python bonus
дата: 2026-07-11
связано:
  - "[[two-sum]]"
  - "[[contains-duplicate]]"
---

# Обёртки над структурами данных в Python / stdlib containers & complexity

## Что это

Семейство контейнеров из `collections` + модуль `heapq` — это **надстройки над `dict` и `list`**, дающие удобный интерфейс и правильную сложность там, где голый список работает за O(n). Новой алгоритмики не добавляют — выигрыш в чистоте кода и в сложности операции.

## Ключевые термины (English)

- **hash table** — таблица с доступом по ключу за среднее O(1); основа `dict` и `set`.
- **open addressing** — способ разрешения коллизий в CPython: при занятом слоте пробируются другие слоты (не цепочки-списки, как в Java).
- **amortized O(1)** — усреднённая по многим операциям стоимость; отдельная может быть дороже (ресайз).
- **hashable** — объект с неизменяемым `hash()`; только такие можно класть в `set`/ключом `dict` (`int`, `str`, `tuple` — да; `list`, `dict` — нет).
- **default factory** — функция-фабрика дефолта в `defaultdict` (`int`→0, `list`→`[]`).
- **min-heap** — куча, где `pop` всегда отдаёт минимум за O(log n); в Python это `list` + `heapq`.
- **early-exit** — выход из цикла при первом же совпадении, не обрабатывая остаток.

## Как работает

### dict / set — базовые хеш-таблицы
`dict` хранит пары `ключ → значение`, `set` — только ключи. Обе — hash table с open addressing. Доступ/вставка/поиск в среднем O(1). `in` смотрит на **ключи**. Ключи обязаны быть hashable.

### Counter — подсчёт частот (над dict)
```python
from collections import Counter
c = Counter("aabbbc")     # Counter({'b': 3, 'a': 2, 'c': 1})
c['z']                    # 0  — отсутствующий ключ, НЕ KeyError
c.most_common(2)          # [('b', 3), ('a', 2)]  — топ-K
Counter(s1) == Counter(s2)  # одинаковый набор с частотами → готовая проверка анаграммы
```
Построение — O(n). `most_common(k)` — O(n log k), целиком — O(n log n).

### defaultdict — dict с дефолтом (над dict)
```python
from collections import defaultdict
d = defaultdict(int)      # дефолт 0
d['x'] += 1               # не падает, хотя ключа не было
g = defaultdict(list)     # дефолт [] — списки смежности графа / группировка
g[node].append(neighbor)
```
Убирает ручное `if key not in d: d[key] = ...`. Операции — как у `dict`, O(1) среднее.

### deque — очередь с двух концов (над двусвязным блочным списком)
```python
from collections import deque
q = deque([1, 2, 3])
q.appendleft(0)   # O(1)
q.popleft()       # O(1)  ← ради этого он и нужен
```
У `list` `pop(0)`/`insert(0, x)` — **O(n)** (сдвиг всех элементов). У `deque` оба конца — O(1). Расплата: доступ к середине `q[k]` — O(n). Главный инструмент BFS (Trees level-order, Graphs).

### heapq — куча поверх list (раздел Heap, забегая вперёд)
```python
import heapq
h = []
heapq.heappush(h, 5)   # O(log n)
heapq.heappop(h)       # минимум, O(log n)
heapq.heapify(nums)    # список → куча на месте, O(n)
```
Min-heap. Для max-heap кладут числа со знаком минус.

## Пример

Из сессии — оба базовых контейнера уже применены:

```python
# Two Sum: dict хранит "число → индекс", ищем дополнение за O(1)
pairs = dict()
for i in range(len(nums)):
    need = target - nums[i]
    if need in pairs:          # in по ключам — O(1)
        return pairs[need], i
    pairs[nums[i]] = i

# Contains Duplicate: set виденных, in/add за O(1)
elements = set()
for el in nums:
    if el in elements:
        return True
    elements.add(el)
return False
```

## Сложность операций (заучить на собес)

```
list:
  l[i]            O(1)      доступ по индексу
  l.append(x)     O(1)*     амортизированно
  l.pop()         O(1)      с конца
  l.pop(0)        O(n)      с начала — сдвиг! → deque
  x in l          O(n)      линейный поиск
  l.insert(i,x)   O(n)
  l.sort()        O(n log n)

dict / set:
  d[k], k in d    O(1)*     среднее; O(n) худшее (коллизии)
  d[k] = v        O(1)*
  del d[k]        O(1)*

deque:
  append / appendleft / pop / popleft   O(1)
  q[k] (середина)                       O(n)

heapq:
  heappush / heappop   O(log n)
  heapify              O(n)
```
`*` — амортизированное / среднее.

## Подводные камни

- **`x in list` — это O(n).** Проверяешь принадлежность внутри цикла → держи данные в `set`/`dict`, иначе внезапная O(n²). Самая частая скрытая деградация.
- **`l.pop(0)` / `insert(0, x)` — O(n).** Для операций с головой бери `deque`, иначе BFS становится O(n²).
- **Ключи должны быть hashable.** `list`/`dict` ключом быть не могут — у изменяемого объекта хеш «уплыл» бы. Кортеж `tuple` можно (если внутри тоже hashable).
- **O(1) — среднее, не гарантия.** При массовых коллизиях (подобранные ключи) `dict`/`set` деградируют к O(n). На этом строят **hash-DoS** (безопасность): злоумышленник шлёт ключи с одинаковым хешом, забивая один probing-кластер — сервер уходит в O(n²) на парсинге. В LeetCode не встретится, но для security-контекста важно.
- **`Counter[missing]` возвращает 0, `dict[missing]` кидает `KeyError`.** Разное поведение на отсутствующем ключе — легко перепутать.

## Связанные темы

[[two-sum]] [[contains-duplicate]]
