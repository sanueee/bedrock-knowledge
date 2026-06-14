#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
    int key;
    int height;
    struct Node *left, *right;
} Node;

static Node *node_new(int key) {
    Node *n = malloc(sizeof(Node));
    if (!n) { perror("malloc"); exit(1); }
    n->key = key;
    n->height = 0;
    n->left = n->right = NULL;
    return n;
}

int height(Node *n){ return n ? n->height : -1; }

int balance_factor(Node *n)
{
    return n ? (height(n->left) - height(n->right)) : 0;
}

void update_height(Node *n)
{
    if (n == NULL) return;
    int lh = height(n->left);
    int rh = height(n->right);
    if (lh >= rh)
        n->height = 1 + lh;
    else
        n->height = 1 + rh;
}

Node *rotate_right(Node *z) {
    Node *y = z->left;
    Node *t = y->right;
    y->right = z;
    z->left  = t;
    update_height(z);
    update_height(y);
    return y;
}

Node *rotate_left(Node *z) {
    Node *y = z->right;
    Node *B = y->left;
    y->left = z;
    z->right = B;
    update_height(z);
    update_height(y);
    return y;
}

Node *rebalance(Node *root) {
    update_height(root);
    int bf = balance_factor(root);
    if (bf > 1 && balance_factor(root->left)  >= 0) return rotate_right(root);          // LL
    if (bf > 1 && balance_factor(root->left)  <  0) {                                   // LR
        root->left = rotate_left(root->left);  return rotate_right(root);
    }
    if (bf < -1 && balance_factor(root->right) <= 0) return rotate_left(root);          // RR
    if (bf < -1 && balance_factor(root->right) >  0) {                                  // RL
        root->right = rotate_right(root->right); return rotate_left(root);
    }
    return root;
}

/*
LL / RR — «прямая палка» (узел и тяжёлый ребёнок перекошены в одну сторону) → один поворот.
LR / RL — «зиг-заг» (узел и ребёнок перекошены в разные стороны) → два поворота:
 сначала поворот ребёнка, чтобы свести зиг-заг к прямой палке (LL или RR), 
 потом обычный поворот вокруг z
*/
Node *avl_insert(Node *root, int key) {
    if (root == NULL)
        return node_new(key);
    else if (root->key > key)
        root->left = avl_insert(root->left, key);
    else if (root->key < key)
        root->right = avl_insert(root->right, key);
    update_height(root);
    return rebalance(root);
}

/* ДАНО (ты писал это в bst.c): минимум поддерева = самый левый узел. */
Node *find_min(Node *root) {
    if (!root) return NULL;
    if (!root->left) return root;
    return find_min(root->left);
}

Node *avl_delete(Node *root, int key) {
    if (!root) return NULL;
    if (key > root->key)
        root->right = avl_delete(root->right, key);
    else if (key < root->key)
        root->left = avl_delete(root->left, key);
    else
    {
        if (root->left == NULL)
        {
            Node *tmp = root->right;
            free(root);
            return tmp;
        }
        else if (root->right == NULL)
        {
            Node *tmp = root->left;
            free(root);
            return tmp;
        }
        else
        {
            Node *tmp = find_min(root->right);
            root->key = tmp->key;
            root->right = avl_delete(root->right, tmp->key);
        }
    }
    if (!root) return NULL;
    return rebalance(root);
}

int is_avl(Node *n) {
    if (!n) return 1;
    int bf = balance_factor(n);
    if (bf < -1 || bf > 1) return 0;
    return is_avl(n->left) && is_avl(n->right);
}

void print_inorder(Node *root) {
    if (!root) return;
    print_inorder(root->left);
    printf("%d ", root->key);
    print_inorder(root->right);
}

void avl_free(Node *root) {
    if (!root) return;
    avl_free(root->left);
    avl_free(root->right);
    free(root);
}

int main(void) {
    Node *root = NULL;
    for (int i = 1; i <= 7; i++)
        root = avl_insert(root, i);

    printf("in-order: ");
    print_inorder(root);
    printf("  (ожидаем 1 2 3 4 5 6 7)\n");
    printf("высота:   %d   (ожидаем 2, не 6)\n", height(root));
    printf("корень:   %d   (ожидаем 4)\n", root ? root->key : -1);
    printf("AVL?      %s\n\n", is_avl(root) ? "да" : "НЕТ");

    /* Удаляем и проверяем, что инвариант держится после каждого delete.
     * Дерево было:   4(2(1,3), 6(5,7)) */
    int to_del[] = {1, 2, 3};   /* удаление слева подкосит баланс → потребует поворотов */
    for (int i = 0; i < 3; i++) {
        root = avl_delete(root, to_del[i]);
        printf("после delete %d:  in-order: ", to_del[i]);
        print_inorder(root);
        printf("  высота=%d  AVL=%s\n", height(root), is_avl(root) ? "да" : "НЕТ");
    }

    avl_free(root);
    return 0;
}
