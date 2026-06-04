---
task: task-c04
type: atomic
block-position: C-4 (C1.3-part2)
тема: Rust глава 6 — enums + match exhaustive + Option<T> + if let
статус: в работе
дата: 2026-06-03
код: rust/scan_report/src/main.rs
---

# task-c04 — Rust ch.6: enums + match + Option + if let

## Перед началом перечитать
- [[structs-methods]] — глава 5: `impl`, три формы receiver, automatic referencing. Методы на enum пишутся ровно так же.
- [[guessing-game-notes]] — открытый вопрос 6 (закрывается этой темой).
- [[types-control-flow]] — `match` как выражение, exhaustiveness.

---

## Зачем эта тема

`enum` + `match` — сердце Rust. Это то, чем Rust заменяет:
- классовую иерархию из ООП (вариант = "подтип", но без наследования),
- `union` + тег из C (но компилятор сам следит за тегом),
- `null` (его нет — вместо него `Option<T>`, и компилятор **заставляет** обработать случай "ничего").

`match` — exhaustive: если ты забыл вариант, код **не скомпилируется**. Это та же философия, что borrow checker — ошибки ловятся до запуска. Для разработчика СЗИ это прямой инструмент: моделировать состояния протокола/сканера так, чтобы "необработанное состояние" было невозможно физически.

Задача намеренно связана с магнум опусом блока B (network scanner) — моделируем результат скана порта.

---

## Задание

Создать новый бинарный crate:

```
cd rust && cargo new scan_report
```

Код — в `rust/scan_report/src/main.rs`.

### Часть 1 — enum с данными в вариантах

Определить enum `PortState`, описывающий результат скана одного TCP-порта. Варианты должны быть **разной формы** (это ключевой момент главы 6 — варианты могут нести разные данные):

```rust
enum PortState {
    Open(String),                 // tuple-like вариант: String — имя сервиса ("ssh", "http")
    Closed,                       // unit-like вариант: данных нет
    Filtered { reason: String },  // struct-like вариант: именованное поле
}
```

Тип, объединяющий порт и его состояние:

```rust
struct PortResult {
    port: u16,
    state: PortState,
}
```

### Часть 2 — метод на enum через `match` (exhaustive)

В `impl PortState` написать метод:

```rust
fn describe(&self) -> String
```

Внутри — `match self { ... }`, который для каждого варианта возвращает человекочитаемую строку:
- `Open(service)` → `"открыт (сервис: ssh)"`
- `Closed` → `"закрыт"`
- `Filtered { reason }` → `"фильтруется: <reason>"`

Требования:
- `match` должен покрывать **все** варианты явно (без `_`) — прочувствуй exhaustiveness. Потом попробуй закомментировать один рукав и посмотри на ошибку компилятора (запиши её текст в vault).
- Обрати внимание на binding: в `Open(service)` имя `service` связывается с содержимым варианта; в `Filtered { reason }` — деструктуризация struct-like варианта.
- `format!` для сборки строки (как `snprintf`, но возвращает `String`).

### Часть 3 — `Option<T>`: поиск без `null`

Дан вектор результатов скана:

```rust
let results = vec![
    PortResult { port: 22,  state: PortState::Open(String::from("ssh")) },
    PortResult { port: 80,  state: PortState::Open(String::from("http")) },
    PortResult { port: 139, state: PortState::Filtered { reason: String::from("no response") } },
    PortResult { port: 443, state: PortState::Closed },
];
```

Написать функцию:

```rust
fn find_port(results: &[PortResult], port: u16) -> Option<&PortResult>
```

Она возвращает `Some(&result)` если порт найден, и `None` если нет. Внутри — обычный `for` по срезу (`&[PortResult]`) с `return Some(...)` при совпадении, в конце `None`. Заметь: возвращаем **ссылку** `&PortResult`, не владение — `Option<&T>`.

### Часть 4 — обработать `Option` двумя способами

В `main`:
1. Через `match` — найти порт 80, и в зависимости от `Some`/`None` напечатать описание или "порт не сканировался".
2. Через `if let` — найти порт 8080 (которого нет), и напечатать что-то только в ветке `Some` (покажи, что `if let` — это сокращение `match` когда интересен один рукав).

Затем — пройти по всем `results` в цикле и напечатать `port + describe()` для каждого.

---

## Что в этом задании главное (на это смотрит ревью)

- **Варианты enum несут разные данные** — не выродить в "C-style enum без полей".
- **`match` exhaustive без `_`** в `describe` — и понимание, *почему* компилятор это требует.
- **`Option<&T>` вместо null** — почему возвращаем ссылку, а не `PortResult` по значению (move из чужого вектора невозможен).
- **Деструктуризация в рукавах** `match` — binding содержимого варианта (`Open(service)`, `Filtered { reason }`).
- **`if let` vs `match`** — когда что уместно.

## Подключения / зависимости
Стандартная библиотека, ничего внешнего. `String`, `format!`, `vec!`, `Option` — в prelude, импорты не нужны.

## Команды
```
cd rust/scan_report
cargo run        # сборка + запуск
cargo build      # только сборка (смотреть warnings)
```

---

## Заметки по ходу (заполняется по итогам через vault-write)

<!-- что заработало не сразу, какие ошибки компилятора увидел, что осталось непонятным -->

## Вопросы которые остались открытыми

<!-- -->
