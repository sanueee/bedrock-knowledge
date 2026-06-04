---
тема: Структуры и методы в Rust / Structs and methods
блок: C — Rust
дата: 2026-05-22
связано:
  - "[[ownership]]"
  - "[[types-control-flow]]"
---

# Структуры и методы / Structs and methods

## Что это

Struct — пользовательский тип данных, группирующий именованные поля. `impl`-блок добавляет к нему **методы** (с receiver `self`) и **associated functions** (без `self`, конвенциональные конструкторы).

## Ключевые термины (English)

- **struct** — composite type, named fields. Variants: named-field, tuple struct, unit-like struct.
- **field** — поле struct, имеет имя и тип.
- **tuple struct** — struct без имён полей: `struct Point(i32, i32);`. Доступ через `.0`/`.1`.
- **unit-like struct** — struct без полей: `struct Marker;`. Используется для типов-маркеров и trait impl.
- **instance** — конкретное значение struct.
- **field init shorthand** — `Self { width, height }` вместо `width: width, height: height` когда имена совпадают.
- **struct update syntax** — `User { email, ..base }` создаёт инстанс из частей `base` + новых полей. **Двигает** (move) не-Copy поля.
- **method** — функция в `impl` с receiver: `&self` / `&mut self` / `self`. Вызывается через `.`.
- **associated function** — функция в `impl` **без** `self`. Вызывается через `Type::name(...)`. Конструкторы — конвенция.
- **`Self`** (большая `S`) — алиас для типа внутри `impl`-блока. `Self == Rectangle` внутри `impl Rectangle`.
- **automatic referencing/dereferencing** — компилятор сам добавляет `&` / `&mut` / `*` под сигнатуру метода, когда вызов идёт через `.`.
- **derive macro** — `#[derive(Trait)]` автоматически реализует trait. `Debug` нужен для `{:?}` и `{:#?}`.
- **`Debug` trait** — форматтер для отладочного вывода. Включается через `#[derive(Debug)]`.

## Как работает

### Объявление и создание

```rust
#[derive(Debug)]
struct Rectangle {
    width: u32,
    height: u32,
}

let r = Rectangle { width: 30, height: 50 };
```

Порядок полей при создании — любой, главное все указаны (или через `..base`).

### Methods vs associated functions

```rust
impl Rectangle {
    fn area(&self) -> u32 { self.width * self.height }     // метод
    fn new(w: u32, h: u32) -> Self { Self { width: w, height: h } }  // associated fn
}
```

- `area` — есть `&self` → вызывается `r.area()`.
- `new` — нет `self` → вызывается `Rectangle::new(30, 50)`. Через `.` нельзя, нет инстанса.

### Три формы receiver

| Receiver | Что делает | Когда |
|----------|-----------|-------|
| `&self`     | shared borrow, read-only | большинство методов (`area`, `can_hold`) |
| `&mut self` | exclusive borrow, мутация | методы которые меняют состояние (`set_width`) |
| `self`      | move — потребляет инстанс | методы которые трансформируют (`into_square`); инстанс после вызова недоступен |

### Automatic referencing — главная путаница

Когда вызываешь `r.area()`, компилятор смотрит сигнатуру метода и **сам** добавляет нужный оператор:

- `area(&self)` → компилятор делает `(&r).area()`.
- `set_width(&mut self)` → `(&mut r).set_width()`.
- `consume(self)` → ничего не добавляет, `r` двигается.

**Поэтому `(&r).area()` — это ровно то же что `r.area()`, просто шумно.** Пиши `r.area()`.

Но это работает **только для receiver** (часть до `.`). Аргументы метода — обычные значения, для них правил automatic нет:

```rust
r1.can_hold(&r2)   // & нужен — это аргумент, не receiver
```

Сигнатура `fn can_hold(&self, other: &Rectangle)` требует `&Rectangle` для `other`. Если передать `r2` без `&` — компилятор попытается **move** `r2` в метод (E0382 при следующем использовании `r2`).

### Field init shorthand

```rust
fn new(width: u32, height: u32) -> Self {
    Self { width, height }   // вместо width: width, height: height
}
```

Работает когда имя параметра == имя поля.

### Отладочный вывод

```rust
#[derive(Debug)]   // без этого {:?} не компилируется (E0277)
struct Rectangle { ... }

println!("{:?}", r);    // одна строка: Rectangle { width: 30, height: 50 }
println!("{:#?}", r);   // многострочный, отступы
dbg!(&r);               // печатает в stderr + позицию в файле + возвращает значение
```

`dbg!(&r)` берёт ссылку чтобы не двигать `r`. Без `&` — `dbg!(r)` сдвинет (если тип не `Copy`).

## Пример (из task-c03)

```rust
#[derive(Debug)]
struct Rectangle {
    width: u32,
    height: u32,
}

impl Rectangle {
    fn new(width: u32, height: u32) -> Self {
        Self { width, height }
    }
    fn area(&self) -> u32 {
        self.width * self.height
    }
    fn can_hold(&self, other: &Rectangle) -> bool {
        other.height <= self.height && other.width <= self.width
    }
    fn square(size: u32) -> Self {
        Self::new(size, size)
    }
}

fn main() {
    let r1 = Rectangle::new(30, 50);
    let r2 = Rectangle::new(40, 60);
    let sq = Rectangle::square(50);

    println!("area = {}", r1.area());                  // automatic ref
    println!("can_hold = {}", r2.can_hold(&r1));       // & у аргумента
}
```

## Подводные камни

- **`(&r).method()` — лишний `&` у receiver.** Automatic referencing сам подставит. Пиши `r.method()`.
- **Забытый `&` у аргумента-структуры → move.** `can_hold(r2)` (без `&`) сдвинет `r2`, и дальше `r2` использовать нельзя (E0382). Сравни с `can_hold(&r2)` — это shared borrow, `r2` остаётся живым.
- **`{:?}` без `#[derive(Debug)]`** → E0277 "Rectangle doesn't implement Debug". `Debug` не появляется сам.
- **`return` + `;` в теле-выражении.** `if X { return true; } false` — это многословно. `bool`-выражение возвращается само: `X` достаточно. Не путать с `;` в конце: `X;` превратит выражение в statement, функция вернёт `()` (E0308).
- **`Self` (с большой) ≠ `self` (с маленькой).** `Self` — тип, `self` — инстанс. Внутри `impl Rectangle` пиши `Self::new(...)`, не `Rectangle::new(...)` (хотя оба работают; `Self` устойчивее при переименовании).
- **Associated function не вызвать через `.`** — `r.new(...)` не работает, у `new` нет `self`. Только `Rectangle::new(...)`.

## Связанные темы

[[ownership]] [[types-control-flow]]
