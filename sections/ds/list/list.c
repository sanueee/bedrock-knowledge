#include <stdio.h>
#include <stdlib.h>

typedef struct node {
    int data;
    struct node *next;
} node;

void add_list(node **list, int x)
{
    node *new = malloc(sizeof(node));
    new->data = x;
    new->next = NULL;

    if (*list == NULL)
    {
        *list = new;
        return;
    }
    
    node *cur = *list;
    while (cur != NULL)
    {
        if (cur->next == NULL)
        {
            cur->next = new;
            return;
        }
        cur = cur->next;
    }
}

void delete_list(node **list, int x)
{
    node *cur = *list;
    node *prev = NULL;
    while (cur != NULL)
    {
        if (cur->data == x) // надо удалить
        {
            if (prev == NULL) // нету предыдущего -> удаляем голову
            {
                node *tmp = cur->next; // запомнить другие узлы через запоминание одного следующего
                free(cur);
                cur = NULL;
                *list = tmp;
                return;
            }
            else {
                node *tmp = cur->next;
                free(cur);
                cur = NULL;
                prev->next = tmp;
                return;
            }
        }
        prev = cur;
        cur = cur->next;
    }
}

void reverse_list(node **list)
{
    node *cur = *list;
    node *prev = NULL;
    while (cur != NULL)
    {
        node *next = cur->next;
        cur->next = prev; // разворот
        prev = cur;
        cur = next;
    }
    *list = prev;
}

void print_list(node *list)
{
    for (node *cur = list; cur; cur = cur->next)
        printf("%d ", cur->data);
    printf("\n");
}

void free_list(node **list)
{
    for (node *cur = *list; cur; )
    {
        node *tmp = cur->next;
        free(cur);
        cur = tmp;
    }
    *list = NULL;
}

int main(void)
{
    node *list = NULL;
    add_list(&list, 1);
    add_list(&list, 2);
    add_list(&list, 3);
    add_list(&list, 4);
    add_list(&list, 5);

    print_list(list);

    delete_list(&list, 3);

    print_list(list);

    reverse_list(&list);

    print_list(list);

    free_list(&list);

    return 0;
}