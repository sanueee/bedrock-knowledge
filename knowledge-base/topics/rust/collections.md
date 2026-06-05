---
тема: Rust — Collections (Vec, String, HashMap) (The Book гл. 8)
блок: C — Rust
дата: 2026-06-04
связано:
  - "[[modules]]"
  - "[[ownership]]"
  - "[[enums-match]]"
---

# Глава 8 — Common Collections

## Что это

Три коллекции std, живущие на куче (heap), растущие в рантайме: `Vec<T>` (динамический массив), `String` (UTF-8 строка), `HashMap<K, V>` (ассоциативный массив).

## Ключевые термины (English)

- **`Vec<T>`** — growable array, данные на heap.
- **`String`** — growable UTF-8 string (owned).
- **`HashMap<K, V>`** — hash table; ключу нужны трейты `Eq + Hash`.
- **`entry` API** — «вид» на ячейку map по ключу; `Entry` = enum `Occupied`/`Vacant`.
- **`or_insert` / `or_insert_with`** — вернуть `&mut V`, вставив дефолт если ключа нет.
- **bounds-checked indexing** — `vec[i]` паникует в рантайме при OOB; `vec.get(i)` → `Option`.

## Как работает

### `Vec<T>`

`Vec::new()` / `vec![...]`, `.push(x)`. Итерация — зависит от владения (см. [[ownership]]):
- `for x in &v` — borrow, `x: &T`, `v` жив после.
- `for x in v` — move/consume, `x: T`, `v` после недоступен.

Доступ по индексу — два варианта:
| | возвращает | при OOB |
|---|-----------|---------|
| `v[i]` | `T` (через `Index`) | **паника в рантайме** (компиляция проходит!) |
| `v.get(i)` | `Option<&T>` | `None` (без паники) |

`v[i]` когда инвариант гарантирует валидность; `v.get(i)` когда индекс может быть невалиден штатно (user input).

### `String`

`String::new()`, `push_str(&str)`, `push(char)`, `format!(...)` (строит `String`). Нельзя индексировать `s[0]` — UTF-8, байт ≠ символ. Число → строка: `n.to_string()`.

Сборка отчёта: накапливать в `String` через `push_str`, печать отдельно (`println!` в `main`) — разделяет «построение» и «вывод». `push_str` хочет `&str`, а `format!` даёт `String` → `&format!(...)` (deref coercion `&String`→`&str`).

### `HashMap<K, V>` + `entry` API

`use std::collections::HashMap;` (не в prelude). Ключу нужны `Eq + Hash` (по хешу — корзина, по `Eq` — точное совпадение); часто ещё `Copy`/`Clone`.

**Центральная идиома — `entry`:**
```rust
map.entry(key).or_insert_with(Vec::new).push(val);
```
1. `.entry(key)` → `Entry` (enum `Occupied`/`Vacant`); берёт ключ **по значению** (для возможной вставки).
2. `.or_insert_with(Vec::new)` → `&mut V`: `Occupied` → ссылка на существующее; `Vacant` → зовёт замыкание, вставляет, даёт ссылку на новое.
3. `.push(val)` → работает с `&mut V`.

`or_insert_with(Vec::new)` (ленивый, замыкание — зовётся только при `Vacant`) vs `or_insert(0)` (eager, готовое значение). Для счётчика — `or_insert(0)`, конструировать нечего.

**Счётчик в map** — нужно разыменование:
```rust
*map.entry(k).or_insert(0) += 1;
```
`or_insert(0)` отдаёт `&mut u32`. `+= 1` к самой ссылке нельзя (нет `AddAssign` для `&mut u32`) — нужен `*`, чтобы дойти до значения. Как `(*ptr)++` в C.

**Почему `entry` лучше `get`+`insert`:** один проход по хешу вместо двух (`contains_key` → `get_mut` → `insert` = 2–3 lookup'а), и оба случая (есть/нет) обработаны типобезопасно без `unwrap`.

### Владение при вставке

`map.entry(host.clone())` — `.clone()` потому что `host` за `&`-ссылкой (`for r in &results` → `r: &ScanResult`), а вынести `String` из заёма нельзя (`String` не `Copy`). `r.port` (`u16`, `Copy`) кладётся без `.clone()`. Если бы `results` передавался по значению — `for r in results` дал бы owned `r`, и `host` двигался бы в map без clone.

## Пример

`scan_aggregator/src/report.rs`:
```rust
fn group_open_ports(results: &[ScanResult]) -> HashMap<String, Vec<u16>> {
    let mut map = HashMap::new();
    for r in results {
        if r.state == PortState::Open {                       // фильтр
            map.entry(r.host.clone()).or_insert_with(Vec::new).push(r.port);
        }
    }
    map
}

fn count_by_state(results: &[ScanResult]) -> HashMap<PortState, u32> {
    let mut map = HashMap::new();
    for r in results {
        *map.entry(r.state).or_insert(0) += 1;                // счётчик через *
    }
    map
}
```

## Подводные камни

- `or_insert_with(u32)` — `u32` это тип, не замыкание/значение. Нужно `or_insert(0)` (значение) или `or_insert_with(|| 0)` (замыкание).
- Забыл `*` в `*counter += 1` → ошибка: `&mut u32` нельзя `+= 1`.
- `map.entry(host)` без `.clone()` когда `host` за `&` → `cannot move out of borrowed content`.
- `{}` (`Display`) на `Vec`/`HashMap`/кастомном enum без `Display` → не компилируется; для `Vec` есть `{:?}` (`Debug`), для своего enum — метод `describe()` или derive `Debug`.
- Порядок итерации `HashMap` недетерминирован — для стабильного вывода сортировать.
- `vec[i]` при OOB паникует в **рантайме** (не на компиляции).

## Связанные темы

[[modules]] [[ownership]] [[enums-match]]
