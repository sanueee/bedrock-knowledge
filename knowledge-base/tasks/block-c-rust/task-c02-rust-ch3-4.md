---
task: task-c02
block: C — Rust
substep: C1.2 — The Book главы 3–4 + программа на slices/ownership
дата начала: 2026-05-18
дата завершения: 2026-05-18
статус: выполнено
---

# task-c02 — Rust: типы, control flow, Ownership

Содержательное ядро Rust. После этой сессии должны закрыться вопросы 1, 2, 3, 5, 7 из [[guessing-game-notes]] — про `mut`, `&mut`, `String` vs `&str`, shadowing, диапазоны. Вопросы 4, 6, 8 (`Result`, `match` exhaustive, traits) закроются дальше.

**Перед началом — прочитать конспекты:**
- [[types-control-flow]] — каркас главы 3 + что отметить.
- [[ownership]] — каркас главы 4 + что отметить.

Они **не заменяют** The Book, а дают мост из C и помогают не утонуть в новой терминологии.

---

## Задание

### Часть 1 — Чтение (пока я отлучусь)

Читать **по The Book**, держа рядом открытыми конспекты [[types-control-flow]] и [[ownership]].

1. **[Глава 3 — Common Programming Concepts](https://doc.rust-lang.org/book/ch03-00-common-programming-concepts.html)** целиком (3.1–3.5).
2. **[Глава 4 — Understanding Ownership](https://doc.rust-lang.org/book/ch04-00-understanding-ownership.html)** целиком (4.1–4.3).

Темп: не пытаться запомнить каждую деталь. Цель — пройти материал и **сформулировать вопросы** там где что-то не сошлось. Любой `cargo new test_xx` чтобы что-то попробовать руками — приветствуется.

### Часть 2 — Программа: `word_tools`

Когда вернёшься. Закрепляет ownership + borrowing + slices + control flow + базовые типы.

**Создать:** `rust/word_tools/` через `cargo new word_tools`.

Программа берёт строку (hardcode на старте — `let text = "the quick brown fox jumps over the lazy dog";`) и реализует **три функции**:

#### 1. `fn first_word(s: &str) -> &str`
Возвращает slice — первое слово (до первого пробела). Если пробелов нет — вся строка.

Ключевая идея: **не аллоцировать**, возвращать slice внутри входной строки. Сигнатура **именно** `&str -> &str` (не `&String`, не `String`).

#### 2. `fn count_words(s: &str) -> usize`
Возвращает количество слов. Можно через `s.split_whitespace().count()` — это **идиоматично**. Можно вручную через байтовый проход — это **поучительно**. Выбрать одно и понять почему.

#### 3. `fn longest_word(s: &str) -> &str`
Возвращает slice — самое длинное слово. Если строка пуста — пустой slice `""`. При равенстве длин — первое из встретившихся.

#### `main`
```rust
fn main() {
    let text = "the quick brown fox jumps over the lazy dog";
    println!("first   : {}", first_word(text));
    println!("count   : {}", count_words(text));
    println!("longest : {}", longest_word(text));
}
```

### Часть 3 — Эксперименты с borrow checker

После того как программа работает — **сломать её** четырьмя способами и записать ошибки компилятора. Это важнее чем сам работающий код: borrow checker учится через провокацию.

В `main.rs` под основным кодом (или в комментарии-описании) попробовать:

1. **Two mutable refs**:
   ```rust
   let mut s = String::from("hello");
   let r1 = &mut s;
   let r2 = &mut s;
   println!("{r1} {r2}");
   ```
   Записать что говорит компилятор.

2. **Mutable + immutable**:
   ```rust
   let mut s = String::from("hello");
   let r1 = &s;
   let r2 = &mut s;
   println!("{r1}");
   ```

3. **Use after move**:
   ```rust
   let s1 = String::from("hi");
   let s2 = s1;
   println!("{s1}");
   ```

4. **Dangling reference**:
   ```rust
   fn dangle() -> &String {
       let s = String::from("oops");
       &s
   }
   ```

Ошибки записать в [[ownership]] в раздел "Сообщения borrow checker'а" (создать его), с одной строкой пояснения "что компилятор хотел сказать".

---

## Хедеры / use'ы

Только стандартная библиотека, никаких внешних крейтов:
```rust
// ничего use'ить не нужно — String, &str, usize, println! — в prelude
```

Никаких `use std::io;` — программа не интерактивная, текст hardcoded.

---

## Где сохранить

- Код: `rust/word_tools/src/main.rs`.
- Vault обновляется через `session-debrief` → `vault-write`:
  - [[types-control-flow]] и [[ownership]] — добавить разделы "что зацепило / что осталось непонятным" по итогам чтения.
  - [[guessing-game-notes]] — пометить закрытые вопросы (1, 2, 3, 5, 7) ссылкой на новые конспекты.
  - При желании — отдельный `topics/rust/slices.md` если slices останутся самым ярким впечатлением.

## Чек по завершении

- [x] Главы 3–4 The Book прочитаны.
- [x] `cargo run` в `word_tools` печатает три строки: first / count / longest.
- [x] Сигнатуры `first_word` и `longest_word` именно `&str -> &str`, без `String` и без аллокаций.
- [x] Borrow checker провоцирован в 4 случаях (просмотрено в IDE, записи перенесены в [[ownership]]).
- [x] Закрытые вопросы (1, 2, 3, 5, 7) помечены в [[guessing-game-notes]].

## Рефлексия (2026-05-18)

Сессия из четырёх итераций `/check`. По итогам:

**Что было сложно (со слов пользователя):**
> Само написание кода пока даётся нелегко — следить что и как изменяется и думать о новых типах не как в C.

Это и есть главный сдвиг при переходе с C на Rust: нужно одновременно держать в голове **тип значения** (`String` vs `&str` vs `&mut str`), **владение** (кто owner), и **состояние мутации** (mut или нет). В C этого слоя нет — там указатель это указатель.

**Ошибки в итерациях (как тренировка):**
- Итерация 1: `counter++` (нет такого), функция без `return`, `let mut result;` без типа, `result[...]` без `&` (unsized).
- Итерация 2: опечатка `=+` вместо `+=` (визуально похоже, парсится как `= +1`).
- Итерация 3: arithmetic underflow `0usize - 1` в индексе среза — типичный pitfall беззнаковых индексов.
- Итерация 4: `count_words` считал пробелы вместо переходов — алгоритм пришлось переформулировать (FSM из двух состояний `in_word: bool`).

Это **классические pitfalls Rust-новичка**, прошли через все. Хорошо что прошли руками, а не получили готовый ответ.

**Что элегантно получилось в `longest_word`:**
Проверка `curr_len > max_len` была вынесена из-под условия пробела — теперь она триггерится на **каждом** не-пробельном символе. Это автоматически закрывает обработку последнего слова в строке (которое не заканчивается пробелом) без отдельного блока после цикла.

**Вынесено в vault:**
- [[types-control-flow]] — конспект главы 3 (immutable by default, shadowing, типы, statements vs expressions, ranges).
- [[ownership]] — конспект главы 4 + раздел "Сообщения borrow checker'а" с реальными ошибками.
- [[guessing-game-notes]] — закрытые вопросы 1, 2, 3, 5, 7 помечены ссылками на конспекты.

**Что дальше:**
По стратегии — чередование. Следующая сессия — **B2 на C** (libpcap, разбор Ethernet/IP/TCP). Rust продолжится на C1.3 (главы 5–6: structs, enums, match) уже после B2.
