---
type: atomic
block-position: C-5
block: C — Rust (параллельный трек)
тема: The Book главы 7–8 — modules/packages + collections (Vec/String/HashMap)
дата: 2026-06-04
статус: выполнено
код: self-practice/rust/rust-ch7-8-modules-collections/scan_aggregator/
связанные: [[enums-match]], [[structs-methods]], [[ownership]], [[guessing-game-notes]]
---

# C-5 — Rust ch.7–8: modules + collections (`scan_aggregator`)

> Перед началом перечитать [[enums-match]] (match как выражение, `Option`, `if let`) и
> [[structs-methods]] (struct + `impl` + receiver). Конспекта по главам 7–8 пока нет —
> создадим `topics/rust/modules.md` и `topics/rust/collections.md` через `vault-write` после сессии.

**Тип:** атомарное C-5 (параллельный трек Rust). Главы 7 (packages/crates/modules/paths/`use`) и 8 (collections: `Vec`, `String`, `HashMap`).

---

## Разминка перед кодом (по итогам Interview 02, 2026-06-04)

Мок-собес вскрыл два пробела, которые сам `scan_aggregator` не закрывает. Закрыть до старта кода (5 минут):

1. **Формы receiver** — перечитать [[structs-methods]]. Держать в голове: **`&mut self` — это exclusive borrow, владение НЕ переходит**; **`self` by value — это move/consume** (оригинал инвалидируется, объект дропается в конце метода). Ссылка = заём, никогда не владение. Микро-проверка: написать в `model.rs` метод `&self` (чтение) и метод `self` (поглощение) — и убедиться что после вызова `self`-метода переменная использоваться не может (E0382).
2. **Почему `String` не `Copy`** — перечитать [[ownership]]. Связка: shallow copy продублировала бы `ptr` → два владельца одного heap-буфера → **double free**. Поэтому move. `.clone()` (трейт `Clone`, **std**) — явная deep copy. `&str` — fat pointer `(ptr, len)`, данные не обязательно на стеке. В коде C-5 ты увидишь это вживую: `map.entry(host.clone())` — `.clone()` нужен именно потому что `String` не копируется молча.

Пробелы 3–4 из Interview 02 (`match`/E0004, `v[i]` runtime panic vs `v.get`) закрываются прямо в этом задании — там, где HashMap отдаёт `Option`, и где можно намеренно словить `E0004`, забыв вариант `PortState`.

**Идея:** взять домен из C-4 (`scan_report` — порты и их состояние) и вырастить его в мини-агрегатор результатов сканирования. На прошлом шаге был один порт — теперь много хостов и портов, которые надо **хранить** (`Vec`), **группировать** (`HashMap`) и **раскладывать по модулям** (`mod`). Это естественно тянет обе главы сразу.

---

## Что написать

Cargo-проект `self-practice/rust/rust-ch7-8-modules-collections/scan_aggregator/`. Принимает (захардкоженный в `main` или собранный в коде) набор результатов сканирования и печатает агрегированный отчёт.

### Структура модулей (глава 7)

Разложить на файлы, не держать всё в `main.rs`:

```
src/
├── main.rs        // точка входа: собирает данные, зовёт report
├── model.rs       // mod model — типы данных
└── report.rs      // mod report — логика агрегации
```

- В `main.rs` объявить `mod model;` и `mod report;` (file-based модули).
- Внутри `report.rs` обращаться к типам через `use crate::model::{...};` (абсолютный путь от корня крейта). Хотя бы один раз увидеть и `super::` (путь к родителю) — например вложенный `mod tests` внутри `report.rs`, который тянет `super::*`.
- Поля структур и сами типы пометить `pub` там, где они пересекают границу модуля. Прочувствовать: модуль приватен по умолчанию, `pub` открывает наружу.

### Типы (глава 5–6, повторение)

В `model.rs`:

```rust
pub enum PortState { Open, Closed, Filtered }   // как в C-4

pub struct ScanResult {
    pub host: String,        // owned, не &str — данные живут в Vec
    pub port: u16,
    pub state: PortState,
}
```

### Агрегация (глава 8)

В `report.rs` написать функции, которые реально используют все три коллекции:

1. **`Vec<ScanResult>`** — входной набор. Пройтись `for r in &results` (borrow, не move — иначе Vec уедет). Применить `.iter()`, `.filter(...)`, `.count()` хотя бы раз (например, посчитать сколько всего открытых портов).

2. **`HashMap<String, Vec<u16>>`** — сгруппировать **открытые** порты по хосту. Ключевая идиома главы 8:
   ```rust
   map.entry(host.clone()).or_insert_with(Vec::new).push(port);
   ```
   Разобраться, **почему** `entry().or_insert*()` лучше, чем «проверить `get`, потом `insert`» (один lookup вместо двух; и borrow checker не даёт сделать «проверил-потом-вставил» наивно).

3. **`String`** — собрать текстовый отчёт в один `String` через `push_str` / `format!`, вернуть его из функции (а не печатать внутри). Печать — в `main`. Это разделяет «построение» и «вывод».

4. **`HashMap<PortState, u32>`** (или подсчёт через match) — гистограмма: сколько портов в каждом состоянии. Если делать `HashMap` по `PortState` как ключу — придётся вывести `#[derive(PartialEq, Eq, Hash)]` на enum: разобраться зачем эти трейты нужны для ключа HashMap.

### Что должен печатать

Примерно:
```
=== Scan summary ===
Hosts scanned: 3
Open ports total: 4

scanme.local: 22, 80, 443
db.local: 5432

State histogram: Open=4 Closed=2 Filtered=1
```

---

## Темы, которые нужно реально потрогать (чек-лист)

Глава 7:
- [ ] `mod model;` / `mod report;` — file-based модули, как компилятор их находит
- [ ] `pub` на типах/полях/функциях — что закрыто по умолчанию
- [ ] `use crate::model::{...}` — абсолютный путь; разница `crate::` / `self::` / `super::`
- [ ] package vs crate vs module: что есть что в твоём проекте (Cargo.toml = package, `src/main.rs` = binary crate root)

Глава 8:
- [ ] `Vec<T>`: `Vec::new()` / `vec![]`, `push`, итерация `&vec` vs `vec` (borrow vs move), `.get(i)` (→`Option`) vs `vec[i]` (паника)
- [ ] `String`: `String::new()`, `push_str`, `format!`, почему нельзя индексировать `s[0]` (UTF-8)
- [ ] `HashMap<K,V>`: `insert`, `get` (→`Option<&V>`), `entry().or_insert_with()`, итерация, ownership при `insert` (move ключа/значения, `Copy`-типы копируются)
- [ ] зачем ключу HashMap трейты `Eq + Hash`

---

## Заголовки/импорты (Rust-аналог)

В Rust нет `#include`, но есть `use`. Что понадобится:
- `use std::collections::HashMap;` — `HashMap` **не** в прелюдии, надо импортировать явно (в отличие от `Vec`/`String`/`Option`, которые в прелюдии).
- `use crate::model::{ScanResult, PortState};` — свои типы между модулями.
- Для `#[derive(...)]` ничего импортировать не нужно.

---

## Где сохранить

Код: `self-practice/rust/rust-ch7-8-modules-collections/scan_aggregator/` (`cargo new scan_aggregator`).
Vault (этот файл): `self-practice/rust/rust-ch7-8-modules-collections/task-c05-rust-ch7-8-modules-collections.md`.

После сессии — `vault-write`: создать `topics/rust/modules.md` и `topics/rust/collections.md`, обновить roadmap (C-5 → выполнено).

---

## Ход выполнения

Написал `scan_aggregator` из трёх модулей (`main` / `model` / `report`), 2026-06-04.

- **`model.rs`** — `enum PortState` (derive `Clone, Copy, PartialEq, Eq, Hash`) + `impl PortState::describe(&self) -> &str` (через `match`, литералы `&'static str` — изначально писал `-> String` с `String::from`, на ревью переделал на `-> &str` без аллокации). `struct ScanResult { host: String, port: u16, state: PortState }`, поля `pub`.
- **`report.rs`** — `count_ports` (фильтр `Open` + счётчик), `group_open_ports` (`HashMap<String, Vec<u16>>` через `entry().or_insert_with(Vec::new).push()`, `host.clone()` — владение через `&` не отдать), `count_by_state` (гистограмма `HashMap<PortState, u32>`, `*counter += 1` через разыменование `&mut u32`), `write_report` (сборка `String` через `push_str`/`format!`, сепаратор-флаг `first` для `N−1` запятых). Наружу только `write_report` — `pub`, остальное приватно (инкапсуляция).
- **`main.rs`** — `mod model; mod report;`, `use`, захардкоженные данные, `println!(write_report(&results))`.

Ревью (`/check`, 2 итерации): 1-я — 4 блокера (мусорный символ `∆`, заглушка `use {...}`, `PortState` без `Display`, пустой `main`); 2-я — баг логики: `group_open_ports` клал **все** порты, а не только `Open` (имя врало, не хватало гварда). Плюс прогон по теории модулей (package/crate/module, пути, `pub`/`E0603`, `use`, prelude).

## Что усвоено / слабые места

**Усвоено:** `entry` API (`or_insert_with` ленивый vs `or_insert` eager), разыменование `&mut` для инкремента счётчика в map, `host.clone()` потому что `String` не вынести из `&`, derive `Eq+Hash` на ключ, file-based модули + `pub`-инкапсуляция, `use` как ярлык имени (не `#include`), prelude.

**Слабые места (со слов пользователя на дебрифе):**
1. **Ownership-терминология** — тянусь к «дропнулось» там, где работает «**отдал владение** (move)». Drop вторичен; компилятор запрещает по факту move'а статически (`E0382`), ещё до рантайма. См. [[ownership]].
2. **package / crate / module** — путал уровни (package спутал с путём в `std`; модуль определял «по отсутствию `main`»). Правильно: package = `Cargo.toml`-бандл ⊃ crate ⊃ module ⊃ item; `main.rs` сам по себе модуль (root = `crate`). См. [[modules]].

Связанные конспекты по итогам: [[modules]], [[collections]], [[ownership]],.
