---
тема: Rust — enums + match + Option / if let (глава 6 The Book)
блок: C — Rust
дата: 2026-06-03
связано:
  - "[[structs-methods]]"
  - "[[ownership]]"
  - "[[types-control-flow]]"
  - "[[guessing-game-notes]]"
---

# Enums + match + Option / if let

## Что это

`enum` в Rust — тип, значение которого — **один из** перечисленных вариантов, причём каждый вариант может **нести свои данные** разной формы. `match` — конструкция-**выражение** для разбора значения по вариантам, с проверкой полноты (exhaustive). `Option<T>` — стандартный enum (`Some(T)` / `None`), которым Rust заменяет `null`.

Этим Rust заменяет сразу три вещи из других языков: классовую иерархию ООП (вариант ≈ подтип, но без наследования), `union`+тег из C (но тег под контролем компилятора), и `null` (его нет — есть `Option`, и компилятор заставляет обработать «ничего»).

## Ключевые термины (English)

- **enum** — sum type; значение = ровно один из вариантов.
- **variant** — конкретный вариант enum'а; бывает unit-like, tuple-like, struct-like.
- **`match`** — exhaustive control-flow expression по образцам.
- **arm** — одна ветка `match`: `pattern => expression`.
- **scrutinee** — значение, которое разбирает `match` (то, что стоит после `match`).
- **pattern** — образец слева от `=>`; сверяется с формой значения, связывает части.
- **binding** — переменная, в которую `match` связывает содержимое варианта.
- **exhaustiveness** — требование покрыть все варианты (иначе compile error).
- **catch-all `_`** — образец «всё остальное»; гасит exhaustiveness.
- **`Option<T>`** — `Some(T)` или `None`; замена `null`.
- **`if let`** — сахар над `match` для одной интересной ветки.
- **match ergonomics** — авто-протаскивание `&`/`&mut` в образцы при матчинге ссылки.
- **tail expression** — последнее выражение блока/функции без `;`; становится возвращаемым значением.

## Как работает

### enum с данными

```rust
enum PortState {
    Open(String),                 // tuple-like: данные позиционно
    Closed,                       // unit-like: данных нет
    Filtered { reason: String },  // struct-like: именованное поле
}
```

Варианты одного enum'а могут быть **разной формы** — это и есть сила enum'а: одно значение `PortState` либо несёт имя сервиса, либо причину фильтрации, либо ничего.

### match — это выражение

Главный сдвиг относительно C: `switch` в C — **statement** (ничего не вычисляет, внутри пишешь `return`/присваивание). `match` в Rust — **expression**: целиком *является* значением.

```rust
fn describe(&self) -> String {
    match self {                                                  // scrutinee = self (&PortState)
        PortState::Open(service)       => format!("открыт ({})", service),
        PortState::Closed              => String::from("закрыт"),
        PortState::Filtered { reason } => format!("фильтр: {}", reason),
    }   // <- последнее выражение функции без ; => это и есть возврат (tail expression)
}
```

Механика по шагам:
1. `match` смотрит на scrutinee (`self`), сверяет сверху вниз с образцами.
2. Первый совпавший arm — вычисляет своё выражение справа от `=>`.
3. Это значение **становится значением всего `match`**.
4. Раз `match` стоит последним в функции без `;` — его значение возвращается (tail expression). Можно и `return match ... ;`, и `let x = match ... ;`.

Правила arm'ов:
- Все arms возвращают **один тип** (тут везде `String`).
- **Exhaustive**: покрыты все варианты. Забыл вариант → compile error `non-exhaustive patterns`. (Защита: добавишь новый вариант в enum — компилятор найдёт все `match`, где забыл его обработать. `_` эту защиту отключает — использовать осознанно.)
- Образцы **связывают** содержимое: `Open(service)` → `service` = строка внутри; `Filtered { reason }` → деструктуризация поля.

### Option вместо null

`Option<T>` — обычный enum stdlib: `Some(T)` | `None`. Функция, которая «может не найти», возвращает `Option`, а не nullable-указатель:

```rust
fn find_port(results: &[PortResult], port: u16) -> Option<&PortResult> {
    for result in results {
        if result.port == port {
            return Some(result);   // нашли
        }
    }
    None                           // не нашли (tail expression)
}
```

Вызывающий **обязан** разобрать оба случая — забыть `None` нельзя, в этом и смысл замены `null`:

```rust
match find_port(&results, 80) {
    Some(r) => println!("80: {}", r.state.describe()),
    None    => println!("80 не сканировался"),
}
```

### if let — одна ветка

Когда интересен только один случай — `if let` вместо `match` с пустым `None => {}`:

```rust
if let Some(r) = find_port(&results, 8080) {
    println!("8080: {}", r.state.describe());
}   // None молча пропущен
```

| | `match` | `if let` |
|---|---------|----------|
| случаев | все (exhaustive) | один интересный |
| проверка полноты | да | **нет** |
| когда | важны 2+ ветки / нужна страховка | важна ровно одна |

Можно `if let ... { } else { }`, но если обе ветки осмысленны — это уже честный `match`.

## Подводные камни

- **`self::Open` ≠ `PortState::Open`.** `self::` — путь к текущему **модулю**, не enum. Образец называет тип enum'а: `PortState::Open(...)`.
- **`println!` в arm'е вместо `format!`** — `println!` возвращает `()`, а функция ждёт `String` → type mismatch. `format!` собирает и возвращает `String`.
- **`"строка"` это `&str`, не `String`** — в arm'е, где тип ветки `String`, нужен `String::from("...")` / `.to_string()`.
- **`;` после `match`/tail-выражения** — отбрасывает значение, функция вернёт `()`. Когда значение используется (возврат/let/аргумент) — `;` решает всё; когда блок ради side-effect и значение `()` — `;` косметика.
- **match ergonomics:** матчишь ссылку (`&PortState`) → биндинги тоже ссылки (`service: &String`). Move из-под shared reference запрещён, поэтому компилятор сам делает биндинг `&T`. Захочешь владение — `.clone()`.
- **`_` гасит exhaustiveness** — удобно, но теряешь защиту «забыл новый вариант». Для своих enum'ов лучше перечислять явно.

## Связанные темы

[[structs-methods]] [[ownership]] [[types-control-flow]] [[guessing-game-notes]]
