---
тема: Rust — типы, переменные, control flow (The Book гл. 3)
блок: C — Rust
дата: 2026-05-18
связано:
  - "[[toolchain]]"
  - "[[guessing-game-notes]]"
  - "[[ownership]]"
статус: конспект перед чтением — закрывает вопросы 1, 5, 7 из [[guessing-game-notes]]
---

# Глава 3 — Common Programming Concepts

Это база языка. После неё конструкции из guessing_game перестают быть магией (кроме `&mut` и `String` — те разбираются в [[ownership]]).

Primary source: [The Book — Chapter 3](https://doc.rust-lang.org/book/ch03-00-common-programming-concepts.html). Этот файл — каркас + сравнение с C + что отметить во время чтения.

---

## 3.1 Variables and Mutability

### Immutable by default

```rust
let x = 5;
x = 6;        // ОШИБКА компиляции: cannot assign twice to immutable variable
```

Чтобы переменная менялась — `mut`:
```rust
let mut x = 5;
x = 6;        // OK
```

**Зачем такой дефолт?** В C `int x = 5; x = 6;` — норма, и компилятор не помогает отличить "переменная намеренно менятся" от "случайно перезаписана". В Rust любое изменение значения после привязки — **явное намерение**, помеченное `mut`. Это даёт:
- Читателю кода ясно где значение стабильно, а где меняется.
- Компилятору больше гарантий для оптимизаций.
- Базу для системы borrow ([[ownership]]) — `&mut T` отличается от `&T` именно правом мутации.

### Constants

```rust
const MAX_POINTS: u32 = 100_000;
```

Не то же что `let`. Отличия:
- `mut` запрещён (всегда immutable).
- Тип **обязателен** (нет inference).
- Должна быть constant expression (вычисляема в compile-time).
- Может быть в любой области видимости, включая глобальную.
- Принято UPPER_SNAKE_CASE.

Аналог `#define` / `static const` в C, но проверяется типами и областями видимости.

### Shadowing

Закрывает **вопрос 5** из [[guessing-game-notes]]:

```rust
let x = 5;
let x = x + 1;     // НОВАЯ переменная, тенящая старую. Не присваивание.
let x = x * 2;     // ещё одна
println!("{x}");   // 12
```

И — главное — тип может меняться:
```rust
let spaces = "   ";              // &str
let spaces = spaces.len();       // usize — НОВАЯ переменная того же имени
```

С `let mut spaces = "   "; spaces = spaces.len();` — это была бы ошибка типов. Shadowing решает класс задач "сначала это строка, теперь это число" без введения `spaces_str` и `spaces_len`.

В C аналога нет (имя в scope уникально). Ближайшее — внутренний блок `{ int x = 5; { int x = 6; ... } }`.

---

## 3.2 Data Types

Rust — **статически типизированный** язык с **inference** (вывод типа из контекста). Аннотация нужна когда компилятор не может вывести однозначно — например `let guess: u32 = "42".parse().expect("...")` (parse — generic, тип нужен явно).

### Scalar types

| Тип | Что | Аналог в C |
|-----|-----|-----------|
| `i8 i16 i32 i64 i128 isize` | целые знаковые | `int8_t`...`intptr_t` |
| `u8 u16 u32 u64 u128 usize` | целые беззнаковые | `uint8_t`...`uintptr_t` |
| `f32 f64` | float / double | `float / double` |
| `bool` | `true` / `false` | `_Bool / stdbool.h` |
| `char` | **скаляр Unicode (4 байта)**, не байт | в C `char` — 1 байт, тут это `u8` |

`usize` / `isize` — размер указателя. Индексы массивов, длины — всегда `usize`.

**Integer overflow** — в debug-сборке паникует, в release — wrapping (по умолчанию). Это другая модель чем C, где signed overflow — UB.

### Compound types

- **Tuple**: `let t: (i32, f64, char) = (500, 6.4, 'x');` — фиксированный набор разнотипных значений. Доступ: `t.0`, `t.1` или destructuring `let (a, b, c) = t;`.
- **Array**: `let a: [i32; 5] = [1,2,3,4,5];` — фиксированный размер, известен в compile-time, на стеке. Не путать с `Vec<T>` (растущий, на куче — глава 8).

В C массив = указатель на буфер + длина "где-то рядом". В Rust массив = тип `[T; N]` где `N` часть типа. Boundscheck при индексации — runtime panic, не UB.

---

## 3.3 Functions

```rust
fn add(x: i32, y: i32) -> i32 {
    x + y    // выражение без `;` — это return
}
```

Ключевое отличие от C — **statements vs expressions**:
- **Statement** — действие, ничего не возвращает (`let x = 5;`).
- **Expression** — возвращает значение (`5`, `x + 1`, блок `{ ... }`, `if`, `match`, function call).

Блок `{ ... }` — выражение, его значение — последнее выражение **без `;`**:
```rust
let y = {
    let x = 3;
    x + 1      // нет `;` — это значение блока
};             // y = 4
```

Точка с запятой превращает expression в statement (значение отбрасывается). Это и есть `return` "по умолчанию" — последняя строка функции без `;` становится возвращаемым значением.

---

## 3.5 Control Flow

### `if` — это выражение

```rust
let x = if condition { 5 } else { 6 };
```

В C было бы тернарное `x = condition ? 5 : 6;` или `if/else` с присваиваниями. В Rust `if` сам возвращает значение. Все ветки **должны быть одного типа**.

Условие должно быть **`bool`**, не `int`. `if x { ... }` где `x: i32` — ошибка. В C `if (x)` работает с любым "не ноль".

### `loop`, `while`, `for`

- `loop { ... break value; }` — бесконечный цикл. `break` может возвращать значение из цикла:
  ```rust
  let result = loop {
      counter += 1;
      if counter == 10 { break counter * 2; }
  };
  ```
- `while condition { ... }` — обычный.
- `for x in collection { ... }` — итератор. Это **основной** цикл в Rust. Индексные `for (i = 0; i < n; i++)` пишут редко.

Range:
- `0..10` — exclusive (закрывает **вопрос 7** из [[guessing-game-notes]]: `..` без `=`).
- `0..=10` — inclusive (то что в `gen_range(1..=100)`).

```rust
for i in 0..5 {
    println!("{i}");    // 0 1 2 3 4
}
```

---

## Что отметить во время чтения главы 3

1. **Почему immutable by default — это не "неудобно", а основа для borrow checker.** К этой мысли вернёмся в [[ownership]].
2. **Shadowing vs `mut`** — это разные инструменты:
   - `mut` — то же значение меняется.
   - shadowing — старое значение уничтожается, появляется новое (возможно другого типа).
3. **Все блоки — выражения.** Это меняет идиоматику: ты возвращаешь значение из `if`/`match`/`{ }`, а не пишешь временную переменную и потом её присваиваешь.
4. **`bool` строгий.** Никаких `if (ptr)` — будет `if !ptr.is_null()` (для raw pointers) или `Option::is_some()` для безопасной обвязки.
5. **Integer overflow в release — wrapping, не UB.** Это **уже** безопаснее чем signed overflow в C.

---

## Ключевые термины (English)

- **immutable by default** — переменные неизменяемы пока не помечены `mut`.
- **shadowing** — переопределение имени новым `let`, возможно с другим типом.
- **type inference** — вывод типа компилятором из контекста.
- **statement / expression** — действие без значения / выражение со значением. Блок и `if` — выражения.
- **wrapping** — переполнение целого с заворачиванием (в release-сборке).
- **exclusive / inclusive range** — `a..b` (без b) / `a..=b` (с b).
