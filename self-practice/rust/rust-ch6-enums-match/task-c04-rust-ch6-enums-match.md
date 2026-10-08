---
task: task-c04
type: atomic
block-position: C-4 (C1.3-part2)
тема: Rust глава 6 — enums + match exhaustive + Option<T> + if let
статус: выполнено
дата: 2026-06-03
код: self-practice/rust/rust-ch6-enums-match/scan_report/src/main.rs
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

Код — в `self-practice/rust/rust-ch6-enums-match/scan_report/src/main.rs`.

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
cd self-practice/rust/rust-ch6-enums-match/scan_report
cargo run        # сборка + запуск
cargo build      # только сборка (смотреть warnings)
```

---

## Моё решение

Написал `enum PortState` с тремя формами вариантов (`Open(String)` / `Closed` / `Filtered { reason }`), метод `describe(&self) -> String` через exhaustive `match`, `find_port(&[PortResult], u16) -> Option<&PortResult>` через `for` + `if` + ранний `return Some(result)`, и `main` с разбором результата через `match` (порт 80) и `if let` (порт 8080) + финальный цикл по всем портам. Скомпилировалось чисто после правок (`/check` — багов 0).

## Что узнал

- **`match` — это выражение (expression):** вычисляется в значение, как `2+2`. Значение выбранного arm'а становится значением всего `match`. Уходит из функции через **tail expression** (последнее выражение без `;`) либо через явный `return`.
- **Анатомия `match`:** после `match` — *scrutinee* (значение, которое разбираешь, у меня `self: &PortState`); слева от `=>` — *pattern* (форма); справа — выражение-результат. Все arms одного типа, exhaustive без `_`.
- **`println!` vs `format!`:** первый печатает и отдаёт `()`, второй собирает и **возвращает** `String`. Сигнатура `-> String` требовала `format!`.
- **`String` vs `&str` в arm'ах:** `"закрыт"` это `&str`, нужен `String::from(...)` чтобы тип сошёлся со всеми ветками.
- **match ergonomics:** матчу `&PortState` → биндинги становятся `&String` (нельзя move из-под shared reference). То же в `for result in results` (`results: &[PortResult]`) → `result: &PortResult`.
- **`if let`** — сахар над `match` для одной интересной ветки; теряет проверку exhaustiveness, поэтому только когда остальные случаи реально не важны.
- **slice `&[T]`** — borrowed view (ptr+len) на коллекцию, как `&str` для `String`. В `main` `for ... in &results` чтобы не сконсумировать `Vec`.
- **Drop / RAII:** значение освобождается, когда из scope выходит его **владелец**. `for x in vec` (owned) — цикл забирает владение, буфер чистится в конце цикла, `vec` после недоступен (move). `for x in &vec` — заём, ничего не чистится, `vec` жив.

## Ошибки и трудности

- **Главное (со слов в дебрифе):** тяжело зашла концепция `match` как выражения — где значение, где `;`, что возвращается. К концу сессии «более-менее», но **нужна практика**, не теория.
- Поначалу путал `self::Open` (путь к модулю) и `PortState::Open` (вариант enum'а).
- Писал `println!` внутри arm'ов вместо `format!` — не возвращал значение.
- `Service` с большой буквы (non_snake_case warning).
- Сначала печатал статичные строки и не вызывал `describe()` → каскад dead_code warnings (`never read` / `never used`) — наглядно показал, что compiler прослеживает реальное использование по цепочке.
- Владение «иногда забываю» — где нужен `&`, где move.

## Что бы сделал иначе

- Сразу схлопнуть однострочные arm'ы без блоков `{}`.
- Единый язык вывода (смешал русский в `describe` и английский в обрамлении).

## Открытые вопросы / на потом

- `Display` trait (глава впереди) — `describe` естественно ляжет в `impl Display for PortState`, тогда `println!("{}", state)` напрямую.
- Когда биндинг `&String`, а нужно владение — `.clone()` / `.to_string()`.
