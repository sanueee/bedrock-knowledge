/* splay.c — splay-дерево (самонастраивающийся BST, без хранимого баланса).
 *
 * Идея: при каждом обращении узел поднимается в корень (операция splay).
 * Гарантия — АМОРТИЗИРОВАННАЯ O(log n), не пооперационная. Узел без height.
 *
 * Сборка: gcc -Wall -Wextra -g -fsanitize=address ds/splay.c -o ds/splay && ./ds/splay
 *
 * Пишешь ВСЕ тела сам. Механика splay (zig / zig-zig / zig-zag) — в чате с диаграммами.
 */
#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
    int key;
    struct Node *left, *right;
} Node;

static Node *node_new(int key) {
    Node *n = malloc(sizeof(Node));
    if (!n) { perror("malloc"); exit(1); }
    n->key = key;
    n->left = n->right = NULL;
    return n;
}

Node *rotate_right(Node *z) {
    Node *y = z->left;
    z->left = y->right;
    y->right = z;
    return y;
}

Node *rotate_left(Node *z) {
    Node *y = z->right;
    z->right = y->left;
    y->left = z;
    return y;
}

Node *splay(Node *root, int key) {
    if (root == NULL || root->key == key) return root;
    if (key < root->key) // смотрим влево
    {
        if (root->left == NULL) return root; // элемента нету
        if (root->left->key == key) // zig
        {
            return rotate_right(root);
        }
        if (key < root->left->key) // zig zig
        {
            if (root->left->left == NULL) return root; // элемента нету
            root->left->left = splay(root->left->left, key); // гарантируем что key в LL
            root = rotate_right(root); // крутим деда 
            return rotate_right(root); // крутим родителя - после первого поворота в root лежит родитель
        }
        if (key > root->left->key) // zig zag
        {
            if (root->left->right == NULL) return root; // элемента нету
            root->left->right = splay(root->left->right, key); // гарантируем что key в LR
            root->left = rotate_left(root->left); // крутим родителя
            return rotate_right(root); // крутим деда
        }
    }
    else if (key > root->key) // смотрим вправо
    {
        if (root->right == NULL) return root; // элемента нету
        if (root->right->key == key) // zig
        {
            return rotate_left(root); // крутим родителя
        }
        if (key > root->right->key) // zig zig
        {
            if (root->right->right == NULL) return root; // элемента нет
            root->right->right = splay(root->right->right, key); // гарантируем что key в RR
            root = rotate_left(root); // крутим деда
            return rotate_left(root); // крутим родителя - после первого поворота в root лежит parent
        }
        if (key < root->right->key) // zig zag
        {
            if (root->right->left == NULL) return root; // элемента нет
            root->right->left = splay(root->right->left, key); // гарантируем что элемент в RL
            root->right = rotate_right(root->right); // крутим родителя
            return rotate_left(root); // крутим деда
        }
    }
    return root;
}

Node *splay_insert(Node *root, int key) {
    if (!root) return node_new(key);
    root = splay(root, key);
    if (root->key == key) return root;
    Node *new = node_new(key);
    if (key < root->key) // корень станет правым поддеревом нового
    {
        new->left = root->left;
        new->right = root;
        root->left = NULL;
    }
    if (key > root->key) // корень станет левым поддеревом нового
    {
        new->right = root->right;
        new->left = root;
        root->right = NULL;
    }
    root = new;
    return root;
}

Node *splay_search(Node *root, int key) {
    root = splay(root, key);
    return root;
}

void print_inorder(Node *root) {
    if (!root) return;
    print_inorder(root->left);
    printf("%d ", root->key);
    print_inorder(root->right);
}

void splay_free(Node *root) {
    if (!root) return;
    splay_free(root->left);
    splay_free(root->right);
    free(root);
}

int main(void) {
    Node *root = NULL;
    for (int i = 1; i <= 7; i++)
        root = splay_insert(root, i);

    printf("in-order: ");
    print_inorder(root);
    printf("  (ожидаем 1 2 3 4 5 6 7 — BST-свойство держится)\n");
    printf("корень после вставок: %d  (последний вставленный = 7)\n", root ? root->key : -1);

    /* Обращаемся к глубокому узлу — splay обязан поднять его в корень. */
    root = splay_search(root, 1);
    printf("корень после search(1): %d  (ожидаем 1 — поднялся в корень)\n",
           root ? root->key : -1);

    root = splay_search(root, 4);
    printf("корень после search(4): %d  (ожидаем 4)\n", root ? root->key : -1);

    printf("in-order снова: ");
    print_inorder(root);
    printf("  (всё ещё 1..7 — порядок не ломается)\n");

    splay_free(root);
    return 0;
}
