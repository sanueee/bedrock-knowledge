#include <stdio.h>
#include <stdlib.h>

#define max(a, b) ((a > b) ? a : b)

typedef struct Node {
    int key;
    struct Node *left;
    struct Node *right;
} Node;

static Node *node_new(int key) {
    Node *n = malloc(sizeof(Node));
    if (!n) { perror("malloc"); exit(1); }
    n->key = key;
    n->left = NULL;
    n->right = NULL;
    return n;
}

int count_nodes(Node *root) {
    if (root == NULL)
        return 0;
    return (1 + count_nodes(root->left) + count_nodes(root->right));
}

int tree_height(Node *root) {
    if (root == NULL)
        return -1;
    int lh = tree_height(root->left);
    int rh = tree_height(root->right);
    return 1 + max(lh, rh);
}

void print_inorder(Node *root) {
    if (root == NULL)
        return;
    print_inorder(root->left);
    printf("%d ", root->key);
    print_inorder(root->right);
}

Node *bst_insert(Node *root, int key)
{
    if (root == NULL)
        return node_new(key);
    if (key < root->key)
        root->left = bst_insert(root->left, key);
    if (key > root->key)
        root->right = bst_insert(root->right, key);
    return root;
}

Node *bst_search(Node *root, int key)
{
    if (root == NULL)
        return NULL;
    if (root->key > key)
        return bst_search(root->left, key);
    if (root->key < key)
        return bst_search(root->right, key);
    return root;
}



void bst_free(Node *root)
{
    if (root == NULL)
        return;
    bst_free(root->left);
    bst_free(root->right);
    free(root);    
}

Node *find_min(Node *root) {
    if (root == NULL)
        return NULL;
    if (root->left == NULL) return root;
    return find_min(root->left);
}

Node *bst_delete(Node *root, int key) {
    if (root == NULL)
        return NULL;
    else if (key > root->key)
        root->right = bst_delete(root->right, key);
    else if (key < root->key)
        root->left = bst_delete(root->left, key);
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
            root->right = bst_delete(root->right, tmp->key);
        }
        return root;
    }
    return root;
}

int main(void) {
    Node *root = NULL;
    root = bst_insert(root, 5);
    root = bst_insert(root, 3);
    root = bst_insert(root, 8);
    root = bst_insert(root, 2);
    root = bst_insert(root, 4);
    root = bst_insert(root, 9);

    printf("count_nodes = %d   (ожидаем 6)\n", count_nodes(root));
    printf("tree_height = %d   (ожидаем 2)\n", tree_height(root));
    printf("in-order:    ");
    print_inorder(root);        /* ожидаем: 2 3 4 5 8 9 */
    printf("  (ожидаем 2 3 4 5 8 9)\n");

    Node *f = bst_search(root, 4);
    printf("search 4:  %s (key=%d)\n", f ? "найден" : "НЕ найден", f ? f->key : -1);
    Node *g = bst_search(root, 99);
    printf("search 99: %s\n", g ? "найден" : "НЕ найден");

    /* delete — три случая. Дерево:  5(3(2,4), 8(_,9)) */
    root = bst_delete(root, 2);   /* A: лист */
    printf("после delete 2:  "); print_inorder(root); printf("  (ожидаем 3 4 5 8 9)\n");

    root = bst_delete(root, 8);   /* B: один ребёнок (только правый, 9) */
    printf("после delete 8:  "); print_inorder(root); printf("  (ожидаем 3 4 5 9)\n");

    root = bst_delete(root, 5);   /* C: двое детей (3 и 9) → преемник 9 */
    printf("после delete 5:  "); print_inorder(root); printf("  (ожидаем 3 4 9)\n");

    bst_free(root);

    return 0;
}
