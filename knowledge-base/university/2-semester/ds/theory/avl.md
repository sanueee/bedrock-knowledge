---
тема: AVL-дерево (самобалансирующийся BST)
блок: DS — Структуры данных (экзамен)
дата: 2026-06-14
связано:
  - "[[trees-bst]]"
  - "[[recursion-bst-c]]"
  - "[[complexity]]"
  - "[[index]]"
---

# AVL-дерево

> Код: `knowledge-base/university/2-semester/ds/practice/avl/avl.c` (insert с балансировкой написан руками). Авторы — Адельсон-Вельский,
> Ландис, 1962. Строится поверх рекурсии BST из [[recursion-bst-c]].

## 1. Определение
BST с **инвариантом баланса**: для КАЖДОГО узла
```
|height(left) − height(right)| ≤ 1
```
После каждой вставки/удаления инвариант восстанавливается **поворотами**.

## 2. Узел на C — поле height
```c
typedef struct Node {
    int key, height;            // height ХРАНИМ (не считаем заново → O(1) проверка баланса)
    struct Node *left, *right;
} Node;
```
Новый узел — лист, `height = 0`. Соглашение: **height(NULL) = −1**, лист = 0 (то же, что в `bst.c`). Эта −1 делает «нет ребёнка» легально вычитаемым; с 0 балансы поехали бы на единицу.

## 3. Три помощника (то, что забывается)
```c
int height(Node *n) { return n ? n->height : -1; }   // ЕДИНСТВЕННЫЙ способ читать высоту

int balance_factor(Node *n) {                         // bf = h(left) − h(right)
    return n ? height(n->left) - height(n->right) : 0;
}
void update_height(Node *n) {                          // пересчёт ПО ДЕТЯМ
    int lh = height(n->left), rh = height(n->right);
    n->height = 1 + (lh > rh ? lh : rh);
}
```
- `bf > 0` left-heavy, `bf < 0` right-heavy, `|bf| ≤ 1` ок, `±2` → нарушение.
- `bf` **локален** (смотрит только на детей), но **информативен глобально**: `height` ребёнка уже вобрала всю глубину под ним. Поэтому проверка баланса O(1), без обхода вниз.
- `update_height` зовётся **снизу вверх на возврате рекурсии** (дети уже актуальны).

## 4. Где срабатывает баланс (разобрано на 1,2,3)
Рекурсия проверяет `bf` на каждом ПРЕДКЕ новой вершины, снизу вверх. Вставка 1,2,3:
```
1(h2)          узел 2: h(L=NULL)=-1, h(R=3)=0 → bf=-1 ОК
 \             узел 1: h(L=NULL)=-1, h(R=2)=1 → bf=-2 НАРУШЕНИЕ → rotate_left(1)
  2(h1)
   \           поворот — у самого НИЖНЕГО несбалансированного предка (тут корень 1)
    3(h0)
```
Частая ошибка: путать стороны и забыть, что у NULL высота −1 (а не 0).

## 5. Повороты — O(1), перецепка указателей + 2 update_height
```c
Node *rotate_right(Node *z) {       // правый: для left-heavy (LL)
    Node *y = z->left, *t = y->right;
    y->right = z; z->left = t;
    update_height(z); update_height(y);   // СНАЧАЛА опустившийся z, ПОТОМ поднявшийся y
    return y;                              // новый корень поддерева → идиома return!
}
Node *rotate_left(Node *z) {        // левый: зеркало (для RR)
    Node *y = z->right, *t = y->left;
    y->left = z; z->right = t;
    update_height(z); update_height(y);
    return y;
}
```
Порядок update важен: высота родителя зависит от высоты ребёнка → сперва нижний.

## 6. Четыре случая (опознать по bf узла и стороне нового ключа)
```
LL: bf >  1 && key < root->left->key    → return rotate_right(root);
RR: bf < -1 && key > root->right->key   → return rotate_left(root);
LR: bf >  1 && key > root->left->key    → root->left  = rotate_left(root->left);   // выпрямить в LL
                                          return rotate_right(root);
RL: bf < -1 && key < root->right->key   → root->right = rotate_right(root->right); // выпрямить в RR
                                          return rotate_left(root);
```
- **LL/RR — «прямая палка»** (узел и тяжёлый ребёнок в одну сторону) → ОДИН поворот.
- **LR/RL — «зиг-заг»** (в разные стороны) → ДВА: сперва поворот *ребёнка* (свести к палке), потом обычный вокруг z.
- Грабли (пойманы при написании): (1) **перепутать условия RR↔RL** (RR это `key >`, RL это `key <`); (2) **потерять return в двойном повороте** — обязательно `root->left = rotate_left(root->left)`, иначе выпрямленное поддерево не подцепится.

Вход 1..7 по возрастанию даёт только правый перекос → срабатывают лишь RR/RL; чтобы увидеть LL/LR — вставлять по убыванию.

## 7. avl_insert целиком
```c
Node *avl_insert(Node *root, int key) {
    if (!root) return node_new(key);                       // база
    else if (key < root->key) root->left  = avl_insert(root->left,  key);  // спуск: child = recurse
    else if (key > root->key) root->right = avl_insert(root->right, key);
    update_height(root);                                   // ДО bf
    int bf = balance_factor(root);
    /* 4 случая LL/RR/LR/RL — см. выше */
    return root;
}
```

## Удаление (avl_delete) + единый rebalance

AVL-delete = **BST-delete (3 случая) + балансировка на возврате**. Три отличия от insert:

1. **Случай опознаём по `bf` РЕБЁНКА, не по ключу.** Ключа нет (мы удаляли), и дисбаланс возникает на стороне, **противоположной** удалению (снесли слева → правое перевесило). Ключ про форму тяжёлого поддерева ничего не говорит — её читает `balance_factor(child)`. «По ключу» — частный трюк insert (ключ сам создал перекос); «по bf ребёнка» — общий метод.
2. **Delete может крутить на НЕСКОЛЬКИХ уровнях вверх** (insert — максимум 1 поворот). Происходит само в рекурсивном хвосте.
3. **`update_height` обязателен** после удаления (поддерево укоротилось) — иначе `bf` по устаревшим высотам → пропущенный/неверный поворот. `is_avl`, читающий те же поля, этот баг маскирует (рапортует «да» по вранью).

Единая балансировка (DRY — обе операции зовут её в конце):
```c
Node *rebalance(Node *root) {
    update_height(root);                  // ВСЕГДА первым (закрывает забытый update в delete)
    int bf = balance_factor(root);
    if (bf >  1 && balance_factor(root->left)  >= 0) return rotate_right(root);   // LL
    if (bf >  1 && balance_factor(root->left)  <  0) {                            // LR
        root->left = rotate_left(root->left);   return rotate_right(root); }
    if (bf < -1 && balance_factor(root->right) <= 0) return rotate_left(root);    // RR
    if (bf < -1 && balance_factor(root->right) >  0) {                            // RL
        root->right = rotate_right(root->right); return rotate_left(root); }
    return root;
}
// avl_insert/avl_delete в конце:  return rebalance(root);  (потерянный return = баг!)
```

### Почему именно такие условия (разбор формул)
- **Порог `bf > 1` / `< -1`** = ровно `bf == ±2`. `bf ∈ {−1,0,1}` — законная норма. Дальше ±2 не уходит: чиним на каждом уровне, накопиться не успевает.
- **Чей bf смотрим:** `bf=+2` → перевесило левое → смотрим левого ребёнка (зеркально справа).
- **Знак bf ребёнка = форма:** ребёнок перекошен в ту же сторону (`>0`) → прямая палка → один поворот; в противоположную (`<0`) → зиг-заг → два.
- **Граница ноля `>= 0` (а не `> 0`):** `bf(child) == 0` (ровный ребёнок) бывает **только в delete**. Для ровного ребёнка достаточно ОДНОГО поворота → ноль клеим к одинарному (LL/RR). Отсюда асимметрия `>=`/`<=`: ноль всегда в сторону одинарного поворота.

## Сложность
```
поиск/вставка/удаление:  O(log n) — ГАРАНТИРОВАННО (не «в среднем», как голый BST)
повороты на 1 вставку:   ≤ 2 (O(1)); высота AVL h ≤ 1.44·log₂(n+2) − 0.328
```
1.44 — из **деревьев Фибоначчи** (минимум узлов в AVL высоты h: N(h)=N(h−1)+N(h−2)+1, растёт как φ^h) → AVL не более чем на ~44% выше идеального.

## AVL vs красно-чёрное (любимый trade-off экзамена)
AVL **строже** сбалансирован → **быстрее поиск**, но **дороже модификация** (больше поворотов). RB допускает больший перекос → дешевле вставка/удаление. Linux-ядро (epoll, CFS, VMA) использует RB — там вставки/удаления частые. AVL хорош, когда поиск доминирует.

## Параллели
- Python — из коробки нет (`dict`/`set` это хеш); порядковые — `sortedcontainers` (сторонний).
- Rust — `BTreeMap`/`BTreeSet` (B-дерево, не AVL, но та же ниша «упорядоченный словарь»).

## Ключевые термины (English)
- **self-balancing BST** — самобалансирующееся дерево поиска.
- **balance factor** — баланс-фактор = h(left) − h(right).
- **rotation (left/right), single/double** — поворот, одинарный/двойной.
- **LL / RR / LR / RL cases** — четыре случая разбалансировки.
- **Fibonacci tree** — дерево Фибоначчи (минимальный AVL заданной высоты).
- **height invariant** — инвариант высоты (|bf| ≤ 1).

## Связанные темы
[[trees-bst]] [[recursion-bst-c]] [[complexity]] [[index]]
