---
task: task-c01
block: C — Rust
substep: C1.1 — The Book главы 1–2
дата начала: 2026-05-17
дата завершения: 2026-05-18
статус: выполнено
---

# task-c01 — Rust: установка + первая программа + Guessing Game

Старт блока C. Цель — поставить toolchain, запустить hello_world через cargo, написать Guessing Game по [главе 2 The Book](https://doc.rust-lang.org/book/ch02-00-guessing-game-tutorial.html). Главы 3–4 (типы, control flow, Ownership) — следующая Rust-сессия.

## Задание

### Часть 1 — Toolchain (глава 1)

1. Поставить Rust через `rustup`:
   ```
   curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
   ```
   Перезапустить shell или `source $HOME/.cargo/env`.
2. Проверить: `rustc --version`, `cargo --version`.
3. Понять разницу:
   - `rustc file.rs` — прямой вызов компилятора (как `gcc`).
   - `cargo new`, `cargo build`, `cargo run` — build system + package manager (нет аналога в C-мире — ближайшее это `make` + `apt`).

### Часть 2 — hello_world через cargo (глава 1)

```
cd /Users/sanueee/vscode_projects/fn2s/rust
cargo new hello_world
cd hello_world
cargo run
```

Посмотреть на:
- `Cargo.toml` — манифест (название, версия, edition, зависимости).
- `src/main.rs` — точка входа (функция `main()` без аргументов).
- `target/` — артефакты сборки. Добавить в `.gitignore` если ещё не там.
- `Cargo.lock` — фиксация версий зависимостей.

### Часть 3 — Guessing Game (глава 2)

Реализовать **точно по главе 2**, читая текст параллельно. Программа:
- Загадывает число 1..=100.
- В цикле читает ввод пользователя.
- Сравнивает: "Too small" / "Too big" / "You win".

```
cargo new guessing_game
```

В `Cargo.toml` добавить зависимость:
```toml
[dependencies]
rand = "0.8"
```

В `src/main.rs` использовать:
- `use std::io;` — ввод/вывод.
- `use std::cmp::Ordering;` — `Less / Greater / Equal`.
- `use rand::Rng;` — trait, без него `.gen_range()` не работает.
- `io::stdin().read_line(&mut guess)` — ввод. Обратить внимание на `&mut`.
- `String::new()` — создать пустую растущую строку.
- `let guess: u32 = guess.trim().parse().expect("...")` — shadowing + аннотация типа + parse.
- `match guess.cmp(&secret) { ... }` — pattern matching.
- `loop { ... break; }` — бесконечный цикл с выходом.

## Что отметить в голове во время чтения

Не нужно понимать всё досконально — глава 2 даёт обзор, детали будут в 3–4. Но **обратить внимание** и записать вопросы:

- **Зачем `&mut guess`?** В C было бы `&guess` (указатель на изменяемое). Что добавляет `mut`?
- **`let mut guess = String::new()`** — почему `let mut`, а не просто `let`?
- **`expect("...")`** — что произойдёт если ввод не число? Чем это отличается от ignored errno в C?
- **`match` vs `if/else`** — почему match exhaustive?
- **Shadowing**: `let guess = ...; let guess: u32 = ...;` — переменная "переопределяется". Это не присваивание. В чём смысл?

Ответы придут в главах 3–4 — сейчас зафиксировать вопросы.

## Где сохранить

- Код: `sections/rust/rust-ch1-2/hello_world/`, `sections/rust/rust-ch1-2/guessing_game/`.
- Добавить корневой `.gitignore` или внутри `rust/`: `target/`, `**/*.rs.bk`.

## Vault после завершения

Через `session-debrief` → `vault-write`:
- `knowledge-base/topics/rust/toolchain.md` — rustup, cargo, rustc, Cargo.toml/lock, edition.
- `knowledge-base/topics/rust/guessing-game-notes.md` — вопросы из раздела "Что отметить", без ответов (ответы — после глав 3–4).
- Глоссарий: crate, trait, shadowing, `mut`, `match`, `Result`, `expect`, `parse`.

## Чек по завершении

- [x] `rustc --version` работает (toolchain уже стоял: rustc 1.94.0, cargo 1.94.0).
- [x] `cargo run` в `hello_world` печатает "Hello, world!".
- [x] `cargo run` в `guessing_game` — играется до победы.
- [x] Вопросы из раздела "Что отметить" выписаны в [[guessing-game-notes]].

## Рефлексия (2026-05-18)

Пользователь:
> Прочитал 1-2 главы, пока всё ок. Много новых концепций и конструкций по сравнению с C, в котором очень простой синтаксис. Cargo — очень круто что это так интегрировано в язык. Стоит разобраться во всём этом на будущих заданиях.

Сессия была обзорной — не было задачи всё понять, была задача поставить toolchain и почувствовать "вкус" языка. Глава 2 даёт код-без-объяснений — это by design, ответы на конкретные вопросы (`mut`, `&mut`, shadowing, `match` exhaustive, `Result`) ждут глав 3–4.

**Вынесено в vault:**
- [[toolchain]] — rustup/rustc/cargo, Cargo.toml/lock, crate, registry.
- [[guessing-game-notes]] — список открытых вопросов после главы 2.

**Что дальше:** C1.2 — главы 3–4 The Book (типы + control flow + Ownership). Главное содержательное место Rust — `Ownership` и `borrow checker`. Готовиться к тому что это потребует больше усилий чем главы 1–2.
