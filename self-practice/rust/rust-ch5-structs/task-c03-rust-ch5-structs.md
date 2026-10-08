---
task: task-c03
title: Rust ch.5 — structs + methods (rectangles)
status: выполнено
date: 2026-05-22
---

## Задание

Прочитать The Book гл. 5 (5.1 Defining/Instantiating Structs, 5.2 Example Program, 5.3 Method Syntax). Написать `self-practice/rust/rust-ch5-structs/rectangles/src/main.rs` со struct `Rectangle { width: u32, height: u32 }`, методами `area(&self)` / `can_hold(&self, &Rectangle)` и associated functions `new(w, h)` / `square(size)`. В `main` создать 3 прямоугольника (один через `square`), вывести через `{:?}`, `{:#?}`, `dbg!`, посчитать площадь и проверить `can_hold`.

---

## Моё решение

Создал проект через `cargo new rectangles`. Struct с `#[derive(Debug)]`. В `impl` четыре функции — `area`, `new` (через field init shorthand), `can_hold`, `square` (переиспользует `Self::new`). В `main` — три инстанса, три формы debug-вывода (`{:?}`, `{:#?}`, `dbg!`), вызовы `area` и `can_hold`.

Прошёл три итерации `/check`:
1. Первая версия — `can_hold` через `if X { return true; } false`, везде `(&rect).method()`, опечатка в выводе строки 58.
2. Вторая — `can_hold` сжал в одно выражение, опечатку исправил, **но automatic referencing не убрал** (читал замечание глазами, не строкой).
3. Третья — убрал `(& )` у receiver'ов, `cargo fmt` сжал `square` в одну строку, шумные комментарии-результаты удалил.

## Ключевые функции

| Функция | Зачем |
|---------|-------|
| `Rectangle::new(w, h)` | associated function — конвенциональный конструктор |
| `Rectangle::square(size)` | associated function — конструктор для квадрата, делегирует в `new` |
| `area(&self)` | метод — площадь, shared borrow |
| `can_hold(&self, other: &Rectangle)` | метод — содержит ли `self` `other`, оба shared borrow |
| `#[derive(Debug)]` | derive macro — генерирует `Debug` для `{:?}` / `{:#?}` / `dbg!` |

## Что узнал

- **Конструктор в Rust — не языковая конструкция.** Это просто associated function без `self`, возвращающая `Self`. Имя `new` — конвенция.
- **`Self` vs `self`.** `Self` (большая) — алиас типа, `self` — инстанс. Внутри `impl` лучше писать `Self::new(...)` и `Self { ... }`, устойчивее при переименовании типа.
- **Field init shorthand** — `Self { width, height }` работает когда имя параметра совпадает с полем.
- **Automatic referencing.** Компилятор сам подставит `&` / `&mut` / `*` под сигнатуру метода — но **только для receiver**. Аргументы требуют явного `&`. `r.can_hold(&r2)`: receiver `r` — без `&` (automatic), аргумент `&r2` — с `&` (явно, иначе move).
- **Три формы receiver:** `&self` (read-only), `&mut self` (мутация), `self` (move/consume). Выбор — по тому что метод делает с инстансом.
- **`{:?}` требует `#[derive(Debug)]`** — иначе E0277.
- **`{:?}` vs `{:#?}` vs `dbg!`.** Первый — одна строка, второй — multi-line с отступами, третий — stderr + позиция файла + возврат значения (можно вставлять прямо в выражения).
- **`dbg!(&x)`, не `dbg!(x)`** — чтобы не двигать non-Copy.

## Ошибки и трудности

**Главная путаница — `&`.** Был неуверен где `&` обязателен, где избыточен. Изначально писал `(&rect).area()` на всякий случай — оборонительный стиль "лучше с `&`, чем без". После двух ревью понял: receiver автоматизируется компилятором, аргументы — нет. Простое правило: `&` нужен у **аргумента**, не у receiver.

**Вторая итерация** — прочитал замечание про automatic referencing, но не исправил. Симптом: читаю замечания глазами, не строкой. На третьей итерации Claude явно подсветил "**НЕ исправлено**" — помогло.

**`can_hold` через `if`.** Инстинкт из C — "сравнение → флаг → return". Не сразу пришло что `bool`-выражение возвращается само и `if` лишний.

**Многословный `square`** — раскинул `Self::new(size, size)` на 4 строки. `cargo fmt` сжал автоматически.

## Что бы сделал иначе

- Прежде чем писать `(&r).method()` — спросить себя: "это receiver или аргумент?". Если receiver — `&` лишний.
- Запускать `cargo fmt` сразу после первой компиляции, не ждать ревью.
- Читать замечания посимвольно, особенно повторные.
- Использовать `dbg!` чаще — оно полезнее `println!("{:?}", ...)` потому что показывает позицию в коде.

## Открытые вопросы

- Когда `self` (move) реально нужен? В этой задаче все методы — `&self`. Builder-паттерн? Конвертация типа?
- Что такое `impl Display` и почему `{}` (без `:?`) требует именно его, а не `Debug`?
- Когда `tuple struct` лучше named-field struct?

## Код

`self-practice/rust/rust-ch5-structs/rectangles/src/main.rs`

## Связанные темы

[[structs-methods]] [[ownership]] [[types-control-flow]]
