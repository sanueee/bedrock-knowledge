---
тема: Rust — Packages, Crates, Modules, `use` (The Book гл. 7)
блок: C — Rust
дата: 2026-06-04
связано:
  - "[[collections]]"
  - "[[ownership]]"
  - "[[structs-methods]]"
  - "[[enums-match]]"
---

# Глава 7 — Managing Growing Projects (модульная система Rust)

## Что это

Способ Rust организовать растущий код: **package ⊃ crate ⊃ module ⊃ item**. Модули задают пространства имён (namespaces) и границы видимости (privacy), `use` сокращает пути.

## Ключевые термины (English)

- **package** — бандл, которым управляет cargo; задаётся `Cargo.toml`. Содержит ≥1 crate + зависимости.
- **crate** — единица компиляции; имеет корневой файл (crate root). Бывает **binary** (`src/main.rs`) или **library** (`src/lib.rs`).
- **crate root** — корневой файл крейта; он же **root module** с именем `crate`.
- **module** — пространство имён внутри крейта, объявляется через `mod`.
- **item** — то что лежит в модуле: `fn`, `struct`, `enum`, `const`, вложенный `mod`...
- **path** — адрес item'а: `crate::model::ScanResult`.
- **`pub`** — делает item видимым за пределами своего модуля.
- **privacy** — по умолчанию всё private для своего модуля.
- **prelude** — набор имён, авто-внесённых в каждый модуль (неявный `use std::prelude::*`).

## Как работает

### Матрёшка: package ⊃ crate ⊃ module ⊃ item

На примере `scan_aggregator`:

| Уровень | Что | Граница | В проекте |
|---------|-----|---------|-----------|
| package | бандл cargo | `Cargo.toml` | `scan_aggregator/` (1) |
| crate | единица компиляции | корневой файл | `scan_aggregator` binary, корень `main.rs` (1) |
| module | пространство имён | `mod` | `crate` (=`main.rs`) + `model` + `report` (3) |
| item | объявление | — | `ScanResult`, `write_report`, `describe`... |

Ключевое, что путалось: **`main.rs` — это тоже модуль** (root module, имя `crate`). Файл становится модулем не «потому что нет `main`», а потому что он **объявлен через `mod`** (`mod model;` в корне). Без `mod model;` компилятор `model.rs` вообще не увидит. Crate бинарный — потому что корень это `main.rs` (для библиотеки был бы `lib.rs`), а не «потому что есть `fn main`».

### Пути: `crate::` / `super::` / `self::`

Аналогия с файловой системой:

| Префикс | Откуда считает | FS-аналог |
|---------|----------------|-----------|
| `crate::` | корень крейта (`main.rs`) | `/` (абсолютный) |
| `super::` | родительский модуль | `../` |
| `self::` | текущий модуль | `./` |

В плоской раскладке (модули висят на корне) `crate::model` и `super::model` из `report.rs` дают одно и то же — родитель `report` это и есть корень. Разойдутся при вложенности модулей.

### Видимость (privacy) + `pub`

Правило по умолчанию: item **private для своего модуля и его потомков (descendants)**, но **не для предков (ancestors)**.

- `main` (корень) — предок `model`. Предок **не видит** приватное потомка.
- Поэтому `model` обязан открыть типы наружу через `pub`, иначе `main` не достучится.
- Ошибка при отсутствии `pub` — `E0603: struct is private`. Это ошибка **видимости** («найдено, но private»), **не** «путь не найден».

**Асимметрия enum vs struct** (частая путаница):
- `pub enum` ⟶ **все варианты автоматически `pub`**.
- `pub struct` ⟶ сама структура публична, но **поля НЕТ** — каждое поле помечается `pub` отдельно.

Инкапсуляция: открывать наружу минимум. В `report.rs` — только `pub fn write_report`; `count_ports`/`group_open_ports`/`count_by_state` приватны (детали реализации).

### `use` — ярлык имени, не `#include`

`use crate::model::{ScanResult, PortState};` **не импортирует и не копирует код**. Он лишь вносит **имя** в текущий scope — короткий псевдоним для пути. Код определения живёт в `model.rs`, компилируется один раз.

Без `use` всё работает — просто пишешь полный путь на каждом месте:
```rust
fn count_ports(results: &[crate::model::ScanResult]) -> usize { ... }
```
То есть `use` — **эргономика, не семантика**. Можно переименовать при конфликте: `use ... as Alias;`.

### Prelude

`Vec`, `String`, `Option`, `Result`, `Box`, базовые трейты — доступны **без** `use`, потому что они в **prelude** (компилятор как будто вставил `use std::prelude::*` в каждый модуль). `HashMap` живёт в `std::collections`, но **в prelude не входит** — поэтому `use std::collections::HashMap;` явно.

## Пример

`scan_aggregator/src/main.rs`:
```rust
mod model;                                  // объявили модули
mod report;
use crate::report::write_report;            // ярлык на функцию
use model::{ScanResult, PortState};         // ярлык на типы

fn main() {
    let results = vec![ /* ScanResult { ... } */ ];
    println!("{}", write_report(&results));
}
```
`report.rs`:
```rust
use crate::model::{ScanResult, PortState};  // тянем типы из соседнего модуля
pub fn write_report(results: &[ScanResult]) -> String { ... }  // pub — наружу
fn count_ports(results: &[ScanResult]) -> usize { ... }        // private — деталь
```

## Подводные камни

- Забыл `mod model;` в корне → `model.rs` не компилируется (его «нет» для крейта).
- Забыл `pub` на типе/поле → `E0603 is private` у вызывающего модуля (предка).
- Думать что `pub struct` открывает поля — нет, поля поштучно.
- Путать `use` с `#include`: в C препроцессор вставляет текст (дублирование), в Rust `use` — только имя в scope, без копирования.
- Считать `main.rs` «не модулем» — он root module (`crate`).

## Сравнение с C

| | C `#include` | Rust `use` |
|---|---|---|
| Что делает | вставляет текст хедера | вносит имя в scope (ярлык пути) |
| Дублирование | да | нет (код компилируется раз) |
| Обязателен для доступа | да | нет (можно полным путём) |

## Связанные темы

[[collections]] [[ownership]] [[structs-methods]] [[enums-match]]
