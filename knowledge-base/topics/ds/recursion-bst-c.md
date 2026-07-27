---
тема: Рекурсия на деревьях + реализация BST на C
блок: DS — Структуры данных (экзамен)
дата: 2026-06-14
связано:
  - "[[trees-bst]]"
  - "[[linked-list]]"
  - "[[memory-problems]]"
  - "[[index]]"
---

# Рекурсия на деревьях + BST на C

> Мини-сессия перед AVL (закрытие слабого места «рекурсию сам бы не написал»).
> Код: `sections/ds/bst/bst.c`. Все функции написаны руками, прожарены.

## Ментальная модель рекурсии — три ингредиента + прыжок веры

```
1. base case (база)      — когда отвечаем сразу, без вызова себя.
                           для дерева почти всегда: if (node == NULL) return ...;
2. recursive case (шаг)  — вызвать СЕБЯ на меньшей задаче (left/right ребёнок).
3. combine (сборка)      — собрать ответ из результатов детей + текущий узел.
```

**Прыжок веры (leap of faith):** когда пишешь `f(node->left)`, НЕ разворачивай рекурсию в голове. Считай, что `f(node->left)` **уже вернул правильный ответ** для левого поддерева. Твоя работа на текущем уровне — только: (а) верная база, (б) верная сборка из готовых ответов детей. Верны эти двое → верна вся рекурсия (это индукция). «Сам бы не написал» = не доверял вызову, пытался проследить весь спуск.

## Два стиля «дать функции изменить связь у вызывающего»

Функция получает **копию** указателя → не может переписать переменную вызывающего напрямую. Два выхода:

- **Двойной указатель `Node**`** — передаём *адрес поля*, меняем `*pp` напрямую (стиль из списков).
- **Возврат поддерева** — `return` обновлённый указатель, вызывающий **обязан** перезаписать: `root = bst_insert(root, k);` снаружи, `root->left = bst_insert(root->left, k);` внутри.

Третьего нет. Если не присвоить возврат — новый узел теряется (утечка), дерево не меняется.

## Различитель: `return recurse(...)` vs `child = recurse(...); return root;`

Главная ловушка (поймана на пробнике):

```
search  — НИЧЕГО не меняет, нужна только находка  → return bst_search(root->left, key);
insert  — МЕНЯЕТ связи: подцепить и вернуть себя  → root->left = bst_insert(...); return root;
delete  — МЕНЯЕТ связи: то же                     → root->left = bst_delete(...); return root;
```

Если в delete написать `return bst_delete(root->left, key)` — вернёшь результат удаления из левого поддерева **как будто это всё дерево** → отбросишь root и правое поддерево, дерево схлопнется. Баг «потерянного / неправильного return».

## Функции (sections/ds/bst/bst.c) — шаблоны

```c
int count_nodes(Node *r) {            // читает: база 0, сборка 1+left+right
    if (!r) return 0;
    return 1 + count_nodes(r->left) + count_nodes(r->right);
}
int tree_height(Node *r) {            // высота = РЁБРА; база -1 (у листа выйдет 0)
    if (!r) return -1;
    int lh = tree_height(r->left), rh = tree_height(r->right);
    return 1 + (lh > rh ? lh : rh);
}
void print_inorder(Node *r) {         // L, узел, R → отсортированный порядок BST
    if (!r) return;
    print_inorder(r->left); printf("%d ", r->key); print_inorder(r->right);
}
Node *bst_insert(Node *r, int k) {    // меняет: child = recurse; return r
    if (!r) return node_new(k);
    if (k < r->key) r->left  = bst_insert(r->left, k);
    else if (k > r->key) r->right = bst_insert(r->right, k);
    return r;
}
Node *bst_search(Node *r, int k) {    // читает: return recurse
    if (!r) return NULL;
    if (k < r->key) return bst_search(r->left, k);
    if (k > r->key) return bst_search(r->right, k);
    return r;
}
void bst_free(Node *r) {              // POST-order: детей раньше родителя (иначе UAF)
    if (!r) return;
    bst_free(r->left); bst_free(r->right); free(r);
}
```

## Удаление — три случая (capstone)

```c
Node *bst_delete(Node *r, int key) {
    if (!r) return NULL;
    else if (key > r->key) r->right = bst_delete(r->right, key);   // спуск
    else if (key < r->key) r->left  = bst_delete(r->left,  key);
    else {                                                          // НАШЛИ
        if (r->left == NULL) {            // A: лист ИЛИ только правый (tmp=NULL ок для листа)
            Node *tmp = r->right; free(r); return tmp;
        } else if (r->right == NULL) {    // B: только левый
            Node *tmp = r->left;  free(r); return tmp;
        } else {                          // C: двое детей
            Node *succ = find_min(r->right);          // in-order преемник (самый левый справа)
            r->key = succ->key;                       // копируем ключ к себе
            r->right = bst_delete(r->right, succ->key); // удаляем преемника ниже
        }
        return r;
    }
    return r;
}
```

**Критично — `else` между спуском и удалением.** Без `else if`/`else` блок трёх случаев выполнится **даже при спуске** (key != r->key) → покалечит текущий узел (для двух детей сработает копирование преемника). Удаление — только в ветке «нашли».

**Случай A через `r->left == NULL`** (без `&& r->right != NULL`) покрывает и лист, и right-only: для листа `tmp = r->right = NULL`, `free`, `return NULL` — узел исчезает. Если писать A с двумя условиями — лист не обработается → утечка + узел остаётся.

**Случай C — почему не зациклится:** преемник (самый левый справа) **не имеет левого ребёнка** по определению → рекурсивное удаление преемника попадает только в A/B, никогда снова в C. Один уровень — и всё.

**Случай C + богатый payload (ловушка dangling/double-free):** копировать только `key` безопасно лишь пока в узле нет **владеемых указателей**. Если есть `char *name` (узел владеет строкой), shallow-copy указателя → два узла владеют одной строкой → `free` преемника делает `r->name` висячим (use-after-free), при удалении `r` позже — double-free. Решения: **deep copy** (`strdup`) либо **перецепить сам узел-преемник** связями (не копировать данные вовсе) — промышленный подход для тяжёлых узлов.

## Сложность

```
count/height/inorder/free:  O(n) по времени (каждый узел 1 раз), форма НЕ важна.
search/insert/delete:       O(h) — h высота; сбаланс. O(log n), вырожденное O(n).
память рекурсии:            глубина стека = h. вырожденное O(n) → риск stack overflow
                            (в ядре/проде обходы часто итеративные со своим стеком).
```

## Ловушка макроса (попутно)

`#define max(a,b) ((a)>(b)?(a):(b))` + дорогой аргумент-вызов → **двойное вычисление**: `max(f(l), f(r))` вызовет победителя ДВАЖДЫ. В рекурсии это **компаундится по уровням** — не «×2», а смена класса: сбаланс. O(n)→O(n^1.585), вырожденное O(n)→O(2ⁿ). Лечение: сохранить в `int lh, rh;` до макроса, или `static inline int max(...)`.

## Ключевые термины (English)

- **base case / recursive case** — база / рекурсивный шаг.
- **leap of faith** — «прыжок веры»: доверять, что рекурсивный вызов уже верен.
- **in-order successor** — следующий по значению (минимум правого поддерева).
- **shallow copy / deep copy** — поверхностная (указатель) / глубокая (данные) копия.
- **dangling pointer / use-after-free / double-free** — висячий указатель / обращение после освобождения / двойное освобождение.
- **stack overflow** — переполнение стека вызовов (глубокая рекурсия).
- **post-order free** — освобождение «дети раньше родителя».

## Связанные темы

[[trees-bst]] [[linked-list]] [[memory-problems]] [[index]]
