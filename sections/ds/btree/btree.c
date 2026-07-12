/* B-дерево порядка M (2-3 дерево при M=3) — search + insert со split.
 *
 * Сборка:  gcc -Wall -Wextra -g ds/btree.c -o btree && ./btree
 *
 * Порядок M=3 выбран специально: совпадает с тем, что ты строил РУКАМИ
 * (последовательность 20 40 10 30 15 35 5 25 50 45). Сможешь сверить вывод
 * программы со своим бумажным деревом узел в узел.
 */
#include <stdio.h>
#include <stdlib.h>

#define M 3   /* порядок: максимум M-1=2 ключа и M=3 ребёнка в узле */

typedef struct BTreeNode {
    int n;                          /* текущее число ключей */
    int keys[M];                    /* отсортированы по возрастанию */
    struct BTreeNode *child[M + 1]; /* n+1 детей; child[i] держит ключи между keys[i-1] и keys[i] */
    int leaf;                       /* 1 = лист (детей нет) */
} BTreeNode;

static BTreeNode *node_new(int leaf)
{
    BTreeNode *node = calloc(1, sizeof(BTreeNode));
    if (!node) { perror("calloc"); exit(1); }
    node->leaf = leaf;
    return node;
}

int btree_search(BTreeNode *node, int key)
{
    if (!node) return 0;
    for (int i = 0; i < node->n; i++) // смотрим ключи в узле
    {
        if (node->keys[i] >= key) // ищем первую позицию где ключ > keys[i]
        {
            if (node->keys[i] == key)
                return 1;
            else if (node->leaf)
                return 0;
            return btree_search(node->child[i], key);
        }
    }
    if (node->leaf) return 0;
    return btree_search(node->child[node->n], key);
}

void split_child(BTreeNode *parent, int i)
{
    BTreeNode *full = parent->child[i];
    int mid = M/2;
    int key_up = full->keys[mid];
    BTreeNode *right = node_new(full->leaf);
    for (int j = mid + 1; j < full->n; j++)
        right->keys[j - mid - 1] = full->keys[j];
    if (!full->leaf)
        for (int j = mid + 1; j <= full->n; j++)
            right->child[j - mid - 1] = full->child[j];
    right->n = M - mid - 1;
    full->n = mid;
    for (int j = parent->n; j > i; j--)        // сдвиг ключей вправо от i
        parent->keys[j] = parent->keys[j-1];
    parent->keys[i] = key_up;
    for (int j = parent->n + 1; j > i + 1; j--)   // детей n+1, сдвиг от i+1
        parent->child[j] = parent->child[j-1];
    parent->child[i + 1] = right;
    parent->n++;
}

void insert_rec(BTreeNode **root, int key)
{
    BTreeNode *p_root = *root; int pos = p_root->n;
    for (int i = 0; i < p_root->n; i++)
    {
        if (p_root->keys[i] >= key)
        {
            if (p_root->keys[i] == key) return; // уже в дереве
            else if (p_root->leaf) { pos = i; break; } // нашли первую позицию
            (void)insert_rec(&(p_root->child[i]), key); // идем в ребенка
            if (p_root->child[i]->n == M) split_child(p_root, i);
            return;
        }
    }
    if (!p_root->leaf) {
        insert_rec(&p_root->child[p_root->n], key);
        if (p_root->child[p_root->n]->n == M) split_child(p_root, p_root->n);
        return;
    }
    // тут мы должны вставить новый ключ с конца на позицию pos
    for (int j = p_root->n; j > pos; j--)
        p_root->keys[j] = p_root->keys[j-1];
    p_root->keys[pos] = key;
    p_root->n++;
}

void btree_insert(BTreeNode **root, int key)
{
    insert_rec(root, key); // вставить в лист и вызвать сплит листа 
    if ((*root)->n == M) // корень переполнился
    {
        BTreeNode *new = node_new(0);
        new->child[0] = *root;
        split_child(new, 0);
        *root = new;
    }
}

void btree_print(BTreeNode *node, int depth)
{
    if (!node) return;

    printf("%*s", depth * 2, "");          /* отступ: depth*2 пробелов */

    printf("[");                            /* ключи узла: [5 10] */
    for (int i = 0; i < node->n; i++)
        printf("%d%s", node->keys[i], i + 1 < node->n ? " " : "");
    printf("]\n");

    if (!node->leaf)                        /* спуститься во всех n+1 детей глубже на 1 */
        for (int i = 0; i <= node->n; i++)
            btree_print(node->child[i], depth + 1);
}

void btree_free(BTreeNode *node)
{
    if (!node) return;
    if (!(node->leaf))
        for (int i = 0; i <= node->n; i++) btree_free(node->child[i]);

    free(node);
}

int main(void)
{
    BTreeNode *root = node_new(1);

    int seq[] = {20, 40, 10, 30, 15, 35, 5, 25, 50, 45};
    int len = (int)(sizeof(seq) / sizeof(seq[0]));

    for (int i = 0; i < len; i++) {
        btree_insert(&root, seq[i]);
        printf("--- после вставки %d ---\n", seq[i]);
        btree_print(root, 0);
    }

    /* ожидаемое финальное дерево (твоё бумажное):
     *        [20]
     *       /    \
     *    [10]    [35 45]
     *   /   \    /  |   \
     * [5][15] [25 30][40][50]
     */

    printf("\n=== поиск ===\n");
    int probes[] = {25, 45, 5, 99, 30, 100};
    for (int i = 0; i < (int)(sizeof(probes)/sizeof(probes[0])); i++)
        printf("search(%d) = %d\n", probes[i], btree_search(root, probes[i]));

    btree_free(root);
    return 0;
}
