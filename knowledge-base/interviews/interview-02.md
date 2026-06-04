# Interview 02 — 2026-06-04

Блок C (Rust), главы 1–8 The Book. Проведён перед заданием C-5 (`scan_aggregator`) по запросу пользователя — проверка теории по пройденному.

## Темы

Покрыты Rust-фундаменты из C-1…C-4 (главы 1–6) + мостик к C-5 (главы 7–8):
- C-2: типы, ownership (move/Copy, `String` vs `&str`), borrow rules (`&mut` XOR `&`)
- C-3: structs + методы, формы receiver
- C-4: enums, `match` как выражение, exhaustiveness, `Option<T>`
- C-5 (prep): collections — `Vec` индексирование (`[]` vs `get`), владение в циклах `for x in v` / `&v`

## Уверенно знает

- **`Option<T>` vs `null`** (Q5): понимает что отсутствие закодировано в системе типов (`T` всегда есть vs `Option<T>` может не быть), и что доступ к значению — только через распаковку (match/if let/unwrap/?). Связал с `HashMap::get → Option`.
- **Правило заимствования** (Q2): `&mut T` XOR любое число `&T`; цель — исключить data race / одновременную read+write.
- **Владение в циклах** (Q7): `for x in v` потребляет (consume), `for x in &v` заимствует (borrow); после consume `v` недоступна.

## Нужно повторить

1. **Формы receiver (`&self`/`&mut self`/`self`)** — [[structs-methods]]. Ошибки: считает что `&mut self` «делает владельцем» (на самом деле — exclusive borrow, владение не переходит); считает `self` by value «shadowing» (на самом деле — **move/consume**, оригинал инвалидируется, объект дропается в конце метода если не возвращён). **Ссылка = заём, владение не переходит никогда; двигает только `self`-by-value.**
2. **Почему `String` не `Copy`** — [[ownership]]. Не назвал причину: shallow (побитовая) копия продублировала бы `ptr` → два владельца одного heap-буфера → **double free** при двойном `drop`. Поэтому `String` использует move (источник инвалидируется). Доп. ошибки: думает что deep copy — отдельный crate (на самом деле `.clone()` / трейт `Clone` из **std**); описал `&str` как «ссылку на строку на стеке» (на самом деле **fat pointer** `(ptr, len)`, данные могут быть в heap / static / стеке).
3. **`match` exhaustiveness** — [[enums-match]]. Не назвал что забытый вариант = **ошибка компиляции `E0004` non-exhaustive patterns** (не runtime). Смазана граница statement vs expression (`let x = 5;` — statement без значения, не «expression с `;`»). Лишняя привязка `match`-как-выражение к `if let`.
4. **Index vs get** — `v[i]` паникует в **рантайме** (не на компиляции, сборка проходит); `v.get(i) → Option<&T>`. Перепутал этап (сказал «запаникует при компиляции»).

## NotebookLM Prompt

---
Я изучаю язык Rust по официальной книге (The Book), главы 1–8. Уже прошёл: ownership (move/Copy, borrow rules &mut XOR &), String vs &str, structs и методы (формы receiver), enums + match + Option, начинаю modules и collections (Vec/HashMap).

На мок-собесе я уверенно ответил по: Option<T> vs null, правило заимствования, владение в циклах for x in v vs &v.

Но у меня пробелы:
1. Формы receiver: я думал что `&mut self` делает метод владельцем объекта (на самом деле это exclusive borrow, владение не переходит), и что `self` by value — это shadowing (на самом деле это move/consume).
2. Почему String не реализует Copy: я не смог объяснить связь shallow copy → два владельца → double free. Также путаю: deep copy String это .clone() из std (а не отдельный crate), и &str это fat pointer (а не «ссылка на строку на стеке»).
3. match exhaustiveness: не знал что забытый вариант enum даёт ошибку компиляции E0004 (а не падение в рантайме). Путаю границу statement vs expression.
4. Индексирование Vec: думал что v[10] паникует на этапе компиляции, а на самом деле компиляция проходит и паника происходит в рантайме.

Пожалуйста, сфокусируй объяснение на этих пробелах. Для каждого:
1. Объясни концепцию точно и без упрощений
2. Покажи минимальный пример кода
3. Задай мне один проверочный вопрос по этой теме
---

## Закрепление

Пункты 3–4 и часть 2 (Option/get) отрабатываются прямо в **C-5 `scan_aggregator`** (HashMap::get → Option, итерация с заёмом, намеренно словить E0004). Пункты 1–2 (receiver'ы, Copy/double-free) C-5 не покрывает — вынесены в мини-разминку в начале файла задания [[task-c05-rust-ch7-8-modules-collections]].
