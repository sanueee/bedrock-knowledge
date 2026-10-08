---
тема: Rust — открытые вопросы после главы 2 The Book
блок: C — Rust
дата: 2026-05-18
обновлено: 2026-06-03 (закрыт 6 — match exhaustive, после task-c04)
связано:
  - "[[toolchain]]"
  - "[[types-control-flow]]"
  - "[[ownership]]"
  - "[[enums-match]]"
статус: вопросы 1, 2, 3, 5, 6, 7 закрыты. Открыты: 4 (Result — глава 9), 8 (traits — глава 10).
---

# Guessing Game — конструкции и открытые вопросы

После главы 2 The Book. Глава даёт обзор, многие вещи "магия" — детали будут в главах 3–4 (типы, control flow, Ownership). Здесь — список конструкций и **вопросов**, которые сознательно оставлены без ответов до глав 3–4.

## Конструкции из guessing_game

```rust
use std::io;
use std::cmp::Ordering;
use rand::Rng;

fn main() {
    let secret_number = rand::thread_rng().gen_range(1..=100);

    loop {
        let mut guess = String::new();
        io::stdin()
            .read_line(&mut guess)
            .expect("Failed to read line");

        let guess: u32 = match guess.trim().parse() {
            Ok(num) => num,
            Err(_) => continue,
        };

        match guess.cmp(&secret_number) {
            Ordering::Less    => println!("Too small!"),
            Ordering::Greater => println!("Too big!"),
            Ordering::Equal   => { println!("You win!"); break; }
        }
    }
}
```

## Открытые вопросы

### 1. `let mut` vs `let` ✅ закрыто в [[types-control-flow]]
По умолчанию переменная **immutable**. `mut` делает её изменяемой. **Вопрос:** зачем такой дефолт? Подозрение — связано с safety и Ownership (глава 4). **Ответ:** дефолт `immutable` — это база для borrow checker'а. `&mut T` имеет смысл только если существует понятие "ровно один обладает правом мутации". Плюс читателю кода ясно где значение стабильно — в C `int x;` ничего не говорит о намерениях.

### 2. `&mut guess` в `read_line` ✅ закрыто в [[ownership]]
Передаём изменяемую ссылку. В C было бы `&buf` — указатель. Что добавляет `mut` в `&mut`? Это часть системы borrow — детали в главе 4. **Ответ:** `&T` — shared reference (read-only, можно N штук). `&mut T` — exclusive (право мутации, ровно одна). `read_line` должна **дописывать** в строку — значит требует `&mut`. Правило 1 `&mut` XOR N `&T` — основа отсутствия data race в safe Rust.

### 3. `String::new()` и почему `String`, а не `&str` ✅ закрыто в [[ownership]]
В Rust два типа строк: `String` (растущая, owned, на куче) и `&str` (срез/view). Глава 4 — слайсы. **Ответ:** `String` — owned, на куче, может расти. `&str` — view (указатель + длина), не владеет данными. `read_line` должна **аллоцировать и дописывать** — `&str` это view, дописывать некуда. Slice (`&str`) подходит для **чтения** части последовательности без аллокации (см. `first_word` в task-c02).

### 4. `expect("...")` vs игнорирование errno в C
`read_line` возвращает `Result<usize, io::Error>` — enum с двумя вариантами. `expect` распаковывает `Ok` или **паникует** с заданным сообщением на `Err`. В C можно молча проигнорировать возврат — компилятор не заметит. Здесь компилятор форсирует обработку Result. Глава 9 — error handling.

### 5. Shadowing: `let guess = ...; let guess: u32 = ...;` ✅ закрыто в [[types-control-flow]]
Вторая `let` не присваивание — это **новая переменная**, тенящая старую. Полезно когда меняется тип (строка → число). Глава 3 — variables. **Ответ:** shadowing ≠ `mut`. `mut` — то же значение меняется. Shadowing — старое уничтожается, появляется новое (возможно другого типа). В C аналога нет — имя в scope уникально.

### 6. `match` exhaustive
```rust
match guess.cmp(&secret_number) {
    Ordering::Less    => ...,
    Ordering::Greater => ...,
    Ordering::Equal   => ...,
}
```
Компилятор требует чтобы покрыты были все варианты enum'а. Забыл `Equal` — не скомпилируется. В C `switch` этого не требует (можно молча упустить case). Глава 6 — enums + match.

### 7. `1..=100` — диапазон ✅ закрыто в [[types-control-flow]]
`..=` — inclusive range, `..` — exclusive. Range — это тип, реализующий итератор. Глава 3 — control flow. **Ответ:** `0..n` (exclusive) — для индексации длиной `n`. `0..=n` (inclusive) — когда обе границы значимы (как `gen_range(1..=100)`).

### 8. Trait `Rng`
`use rand::Rng;` обязателен, иначе `.gen_range()` не работает. **Trait** — это набор методов, который можно "включить" импортом. Близкая аналогия — C++ interface, но шире. Глава 10 — traits.

## Впечатление пользователя (2026-05-18)

> Много новых концепций и конструкций по сравнению с C, в котором очень простой синтаксис. Cargo — очень круто что это так интегрировано в язык. Стоит разобраться во всём этом на будущих заданиях.

Согласован вектор: главы 3–4 — следующая Rust-сессия. Без них guessing_game остаётся "магией".

## Ключевые термины (English)

- **immutable by default** — переменные не меняются без `mut`.
- **shadowing** — переопределение переменной новым `let`, в т.ч. с другим типом.
- **trait** — набор методов, который тип может реализовать; должен быть `use`-нут чтобы методы стали видны.
- **Result<T, E>** — enum для возврата ошибок (`Ok(T)` / `Err(E)`).
- **panic** — необработанное падение программы (через `expect`/`unwrap` или `panic!`).
- **owned vs borrowed** — `String` (owned) vs `&str` (borrowed view).
