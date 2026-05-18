---
тема: Rust — Ownership, References, Slices (The Book гл. 4)
блок: C — Rust
дата: 2026-05-18
связано:
  - "[[types-control-flow]]"
  - "[[guessing-game-notes]]"
статус: конспект перед чтением — закрывает вопросы 2, 3 из [[guessing-game-notes]]
---

# Глава 4 — Understanding Ownership

**Главная глава Rust.** Здесь язык расходится с C/C++/Java. Без её понимания дальнейшее чтение The Book превращается в "почему компилятор опять ругается".

Primary source: [The Book — Chapter 4](https://doc.rust-lang.org/book/ch04-00-understanding-ownership.html).

Цель этого файла — каркас + объяснение **почему** Ownership вообще нужен (контекст из C-опыта) + что отметить во время чтения.

---

## Зачем вообще Ownership — мост из C

В C у каждого куска памяти есть невидимая ответственность: "кто `free`-ит?" Если этой ответственности нет — утечка. Если двое — double free. Если используешь после free — use-after-free.

Языки решают это по-разному:
- **C/C++** — программист отвечает руками. Дисциплина + Valgrind/ASan.
- **GC-языки** (Java, Go, Python) — рантайм отслеживает живые ссылки и собирает мусор. Платишь — задержки GC, runtime overhead.
- **Rust** — **отслеживание на этапе компиляции**. Компилятор знает кто владеет памятью и когда её освобождать. Нет GC. Нет ручного `free`. Нет use-after-free (это compile error, не crash).

Ownership — это **статическая, compile-time модель управления ресурсами**. Распространяется не только на память, но и на любой ресурс с финализацией (файловые дескрипторы, мьютексы, сокеты).

---

## 4.1 What Is Ownership?

Три правила:

1. У каждого значения **есть владелец** (единственная переменная).
2. **В каждый момент времени — ровно один владелец.**
3. Когда владелец уходит из scope — значение **drop**-ается (вызывается деструктор, память освобождается).

### Stack vs Heap — короткий ликбез

- **Stack** — фиксированные размеры, известные в compile-time. LIFO. Быстро.
- **Heap** — динамические размеры. Аллокатор ищет место, возвращает указатель. Медленнее.

В Rust типы вроде `i32`, `bool`, `char`, фиксированные массивы `[T; N]`, кортежи примитивов — живут на стеке. Типы с динамическим размером (`String`, `Vec<T>`, `Box<T>`) — данные на куче, на стеке только метаданные (указатель + длина + capacity).

### `String` vs `&str` — закрывает вопрос 3 из [[guessing-game-notes]]

```rust
let s1 = String::from("hello");   // owned, на куче — растущая
let s2: &str = "hello";           // строковый литерал, &str — view в read-only память
```

| | `String` | `&str` |
|---|----------|--------|
| Где данные | heap | где угодно (часто .rodata бинаря) |
| Владение | owned | borrowed (view) |
| Размер | растёт | фиксирован на момент создания view |
| Аналог в C | `malloc`-нутый буфер | указатель в `const char*` без перевыделения |

Это **разные типы**. `read_line` принимает `&mut String` потому что должен **дописывать** — `&str` это view, дописывать некуда.

### Move

```rust
let s1 = String::from("hello");
let s2 = s1;            // НЕ копирование. Это MOVE.
println!("{s1}");       // ОШИБКА: borrow of moved value: `s1`
```

После `let s2 = s1` — владелец `String` перешёл к `s2`. `s1` становится невалидным (компилятор не даёт им пользоваться). Когда `s2` уйдёт из scope — `drop` вызовется ровно один раз.

В C `Foo* s2 = s1;` — два указателя на одну память, double-free ждёт. В Rust такая ситуация **исключена на этапе компиляции**.

### Copy

Типы с известным фиксированным размером на стеке (`i32`, `bool`, `char`, `(i32, bool)`...) реализуют trait `Copy` — присваивание копирует биты, оба варианта продолжают существовать:

```rust
let x = 5;
let y = x;
println!("{x}");      // OK — i32 is Copy
```

Правило: если тип владеет ресурсом на куче (`String`, `Vec`) — это move. Если только стек — это copy.

### Передача в функцию = тот же move/copy

```rust
fn takes(s: String) { ... }   // s — новый владелец

let s = String::from("hi");
takes(s);
// s уже невалиден — был перемещён в функцию.
```

Чтобы функция могла **посмотреть, но не забирать** — нужны references.

---

## 4.2 References and Borrowing

Закрывает **вопрос 2** из [[guessing-game-notes]].

```rust
fn len(s: &String) -> usize {
    s.len()
}

let s = String::from("hi");
let n = len(&s);     // одолжили ссылку, не отдали владение
println!("{s}");      // OK — s всё ещё владелец
```

`&s` — **shared reference** (`&T`). Read-only. Можно иметь сколько угодно одновременно.

### `&mut` — exclusive reference

```rust
fn append(s: &mut String) {
    s.push_str(" world");
}

let mut s = String::from("hello");
append(&mut s);     // одолжили **изменяемую** ссылку
```

`&mut T` — **mutable reference**. Право мутации. Из этого вытекает **главное правило borrow**:

> В любой момент времени для значения может существовать **либо** один `&mut T`, **либо** сколько угодно `&T`. Никогда — оба одновременно.

Это причина почему `read_line(&mut guess)` — `&mut`: `read_line` будет **писать** в строку. И никто другой одновременно не может читать `guess` через `&guess`. Это компилятор гарантирует.

### Почему это не "просто строгий стиль"

Это **причина** почему в Rust нет data race без `unsafe`:
- Data race = одновременная запись + чтение/запись из разных потоков.
- Запись требует `&mut T`.
- `&mut T` exclusive — никаких параллельных ссылок.
- Следовательно, data race недостижим компилятором (для безопасного кода).

Та самая мысль из C-блока про pthreads и mutex — здесь решена системой типов, без runtime-проверок.

### Dangling references

В C можно вернуть указатель на стековую переменную — UAF.
```c
char* bad() {
    char buf[16];
    return buf;     // указатель на мёртвую память
}
```

В Rust это **compile error**:
```rust
fn bad() -> &String {        // ошибка: missing lifetime specifier
    let s = String::from("x");
    &s
}
```

Компилятор требует **lifetime**'ы (глава 10) — гарантию что ссылка не переживёт владельца.

---

## 4.3 Slices

**Slice** — view в часть последовательности. Не владеет данными.

```rust
let s = String::from("hello world");
let hello: &str = &s[0..5];    // view в первые 5 байт
let world: &str = &s[6..11];
```

`&str` — это **string slice**, всегда либо view в `String`, либо ссылка на литерал. Внутри — указатель + длина.

Аналогично для массивов:
```rust
let a = [1, 2, 3, 4, 5];
let slice: &[i32] = &a[1..3];   // view [2, 3]
```

### Зачем нужны slices

Классический паттерн из главы 4.3 — "вернуть первое слово строки":

```rust
fn first_word(s: &String) -> &str {
    let bytes = s.as_bytes();
    for (i, &b) in bytes.iter().enumerate() {
        if b == b' ' {
            return &s[..i];
        }
    }
    &s[..]
}
```

Возвращается **slice внутри `s`**. Никакой аллокации, никакого копирования. Компилятор гарантирует через lifetime что `s` живёт пока живёт возвращённый slice.

В C аналог — вернуть `char*` + `size_t length`, и **надеяться** что вызывающий не освободит исходный буфер раньше времени. В Rust — это статически невозможно.

### Лучшая сигнатура — принимать `&str`, не `&String`

```rust
fn first_word(s: &str) -> &str { ... }
```

Теперь функция работает и со `&String` (через **deref coercion** — `&String` неявно конвертируется в `&str`), и со строковыми литералами. Это идиоматично.

---

## Что отметить во время чтения главы 4

1. **Move ≠ shallow copy.** После move старая переменная становится invalid — компилятор не даст ей пользоваться. Это не runtime-флаг "moved", это compile-time анализ.
2. **`Copy` vs `Clone`.** Маленькие стековые типы — implicit copy. Глубокая копия `String` требует явного `.clone()` (платишь аллокацию **видимо**, не магически).
3. **Правило borrow: 1 `&mut` XOR N `&`.** Запомнить это как мантру. 90% сообщений borrow checker'а — про это правило.
4. **Slice = указатель + длина, без владения.** Идея напоминает `(char*, size_t)` пары из C, но safe by construction.
5. **References — это не указатели "ослабленные".** Это указатели с **проверяемой компилятором** дисциплиной. Указатели в Rust тоже есть (`*const T`, `*mut T`) — но требуют `unsafe`.

---

## Связь с предыдущим C-материалом

- **Refcount struct file в ядре** (см. [[fd-kernel-model]]) — runtime-аналог Ownership/borrow для FD. Closing fd = `drop`. `dup` = "клонировать" с увеличением refcount.
- **Race condition в [[pthreads-linux]]** — там data race лечится мьютексом в runtime. В Rust `&mut T` xor `&T` лечит **тот же класс багов** в compile-time для single-thread/multi-thread единообразно.

---

## Сообщения borrow checker'а (из task-12)

Запись четырёх провокаций для будущей подстановки шаблонов. Когда увидишь что-то похожее — узнаваешь паттерн сразу.

### 1. Two mutable refs одновременно

```rust
let mut s = String::from("hello");
let r1 = &mut s;
let r2 = &mut s;
println!("{r1} {r2}");
```

```
error[E0499]: cannot borrow `s` as mutable more than once at a time
 --> src/main.rs
  |
  | let r1 = &mut s;
  |          ------ first mutable borrow occurs here
  | let r2 = &mut s;
  |          ^^^^^^ second mutable borrow occurs here
  | println!("{r1} {r2}");
  |            -- first borrow later used here
```

**Что говорит компилятор:** `&mut T` exclusive. Пока `r1` жив (используется ниже), нельзя сделать второй `&mut`. Если бы `r1` не использовался после — NLL (non-lexical lifetimes) разрешил бы.

### 2. Mutable + immutable одновременно

```rust
let mut s = String::from("hello");
let r1 = &s;
let r2 = &mut s;
println!("{r1}");
```

```
error[E0502]: cannot borrow `s` as mutable because it is also borrowed as immutable
 --> src/main.rs
  |
  | let r1 = &s;
  |          -- immutable borrow occurs here
  | let r2 = &mut s;
  |          ^^^^^^ mutable borrow occurs here
  | println!("{r1}");
  |            -- immutable borrow later used here
```

**Что говорит компилятор:** правило borrow в чистом виде — либо N `&T`, либо 1 `&mut T`, не оба одновременно. Логика: если позволить и то и другое, читатель `r1` может видеть данные в момент когда `r2` их меняет (data race в одном потоке — inconsistent view).

### 3. Use after move

```rust
let s1 = String::from("hi");
let s2 = s1;
println!("{s1}");
```

```
error[E0382]: borrow of moved value: `s1`
 --> src/main.rs
  |
  | let s1 = String::from("hi");
  |     -- move occurs because `s1` has type `String`, which does not implement the `Copy` trait
  | let s2 = s1;
  |          -- value moved here
  | println!("{s1}");
  |            ^^ value borrowed here after move
```

**Что говорит компилятор:** `String` не `Copy` — присваивание это move. После move `s1` "пуст" с точки зрения системы типов, любое использование запрещено. С `i32` или другим `Copy`-типом ошибки не было бы.

### 4. Dangling reference

```rust
fn dangle() -> &String {
    let s = String::from("oops");
    &s
}
```

```
error[E0106]: missing lifetime specifier
 --> src/main.rs
  |
  | fn dangle() -> &String {
  |                ^ expected named lifetime parameter
  |
help: this function's return type contains a borrowed value, but there is no value
      for it to be borrowed from
```

Если добавить lifetime `&'static`:

```
error[E0515]: cannot return reference to local variable `s`
 --> src/main.rs
  |
  |     &s
  |     ^^ returns a reference to data owned by the current function
```

**Что говорит компилятор:** `s` живёт только пока работает функция. После `return` `s` дропнется, и ссылка станет dangling. В C это UB-ловушка, тут — compile error до запуска.

---

## Ключевые термины (English)

- **ownership** — система compile-time управления ресурсами.
- **move** — передача владения; источник становится invalid.
- **borrow** — одалживание (через `&T` или `&mut T`).
- **shared reference** (`&T`) — read-only, можно много.
- **mutable reference** (`&mut T`) — exclusive, ровно один.
- **slice** (`&[T]`, `&str`) — view в часть последовательности, не владеет.
- **drop** — освобождение ресурса при выходе из scope (деструктор).
- **lifetime** — compile-time гарантия что ссылка не переживёт владельца (детально — глава 10).
- **`Copy` / `Clone`** — implicit bit-copy / explicit `.clone()` для глубокой копии.
- **deref coercion** — неявное преобразование `&String` → `&str`, `&Vec<T>` → `&[T]`.
- **dangling reference** — ссылка на освобождённую память (compile error в safe Rust).
- **data race** — одновременная запись + чтение из разных потоков; недостижим в safe Rust по построению.
