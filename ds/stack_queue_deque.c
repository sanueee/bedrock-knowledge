#include <stdio.h>
#include <stdlib.h>

typedef struct node {
    int data;
    struct node *next;
} node;

typedef struct dnode {
    int data;
    struct dnode *prev;
    struct dnode *next;
} dnode;

typedef struct {
    node *top;
    size_t size;
} stack;

typedef struct {
    node *head;
    node *tail;
    size_t size;
} queue;

typedef struct {
    dnode *head;
    dnode *tail;
    size_t size;
} deque;

void push_stack(stack *s, int x)
{
    node *new_node = malloc(sizeof(node));
    new_node->data = x;
    new_node->next = s->top;
    s->top = new_node;
    (s->size)++;
}

void pop_stack(stack *s)
{
    if (s == NULL || s->size == 0)
    {
        printf("error\n");
        return;
    }

    printf("pop=%d ", s->top->data);
    node *tmp = s->top->next;
    free(s->top);
    s->top = tmp;
    s->size--;
}

void peek_stack(stack *s)
{
    if (s->size == 0)
    {
        printf("error\n");
        return;
    }
    printf("peek=%d ", s->top->data);
}

void size_stack(stack *s)
{
    printf("size=%zu\n", s->size);
}

void enqueue(queue *q, int x)
{
    node *new_node = malloc(sizeof(node));
    new_node->data = x;
    new_node->next = NULL;

    if (q->size == 0)
    {
        q->head = new_node;
        q->tail = new_node;
        q->size++;
        return;
    }

    q->tail->next = new_node;
    q->tail = new_node;    
    q->size++;
}

void dequeue(queue *q)
{
    if (q == NULL || q->size == 0)
    {
        printf("error\n");
        return;
    }
    
    printf("get=%d ", q->head->data);
    node *tmp = q->head->next;
    free(q->head);
    q->head = tmp;
    q->size--;
    if (q->size == 0)
        q->tail = NULL;
}

void front_queue(queue *q)
{
    if (q == NULL || q->size == 0)
    {
        printf("error\n");
        return;
    }
    printf("get front=%d ", q->head->data);
}

void size_queue(queue *q)
{
    printf("size=%zu\n", q->size);
}

void reverse_queue(queue *q)
{
    node *prev = NULL;
    node *cur = q->head;
    node *old_head = q->head;
    while (cur != NULL)
    {
        node *next = cur->next; // не потеряли
        cur->next = prev; // развернули
        prev = cur; // сдвиг
        cur = next; // сдвиг
    }
    q->head = prev;
    q->tail = old_head;
}

void push_front_deque(deque *d, int x)
{
    dnode *new_dnode = malloc(sizeof(dnode));
    new_dnode->data = x;
    new_dnode->prev = NULL;
    new_dnode->next = NULL;

    if (d->size == 0)
    {
        d->head = new_dnode;
        d->tail = new_dnode;
        d->size = 1;
        return;
    }

    new_dnode->next = d->head;
    d->head->prev = new_dnode;
    d->head = new_dnode;
    d->size++;
}

void push_back_deque(deque *d, int x)
{
    dnode *new_dnode = malloc(sizeof(dnode));
    new_dnode->data = x;
    new_dnode->prev = NULL;
    new_dnode->next = NULL;

    if (d->size == 0)
    {
        d->head = new_dnode;
        d->tail = new_dnode;
        d->size = 1;
        return;
    }

    new_dnode->prev = d->tail;
    d->tail->next = new_dnode;
    d->tail = new_dnode;
    d->size++;
}

void pop_front_deque(deque *d)
{
    if (d->size == 0)
    {
        printf("error\n");
        return;
    }

    printf("%d", d->head->data);
    dnode *tmp = d->head->next;
    free(d->head);
    d->head = tmp;
    if (d->head)
        d->head->prev = NULL;
    d->size--;
    if (d->size == 0)
        d->tail = NULL;
}

void pop_back_deque(deque *d)
{
    if (d->size == 0)
    {
        printf("error\n");
        return;
    }

    printf("%d\n", d->tail->data);
    dnode *tmp = d->tail->prev;
    free(d->tail);
    d->tail = tmp;
    if (d->tail)
        d->tail->next = NULL;
    d->size--;
    if (d->size == 0)
        d->head = NULL;
}

void peek_front_deque(deque *d)
{
    if (d->size == 0)
    {
        printf("error\n");
        return;
    }
    printf("%d\n", d->head->data);
}

void peek_back_deque(deque *d)
{
    if (d->size == 0)
    {
        printf("error\n");
        return;
    }
    printf("%d\n", d->tail->data);
}

void size_deque(deque *d)
{
    printf("%zu\n", d->size);
}

void reverse_deque(deque *d)
{
    dnode *cur = d->head;
    while (cur != NULL)
    {
        dnode *forward = cur->next;
        dnode *tmp = cur->prev;
        cur->prev = cur->next;
        cur->next = tmp;
        cur = forward;
    }
    dnode *tmp = d->head;
    d->head = d->tail;
    d->tail = tmp;
}

int main(void)
{
    /* ---- стек: жду 3 / 3 2 1 / error ---- */
    stack s = { NULL, 0 };
    printf("stack\n");
    push_stack(&s, 1);
    push_stack(&s, 2);
    push_stack(&s, 3);
    size_stack(&s);      /* 3 */
    peek_stack(&s);      /* 3 */
    pop_stack(&s);       /* 3 */
    pop_stack(&s);       /* 2 */
    pop_stack(&s);       /* 1 */
    pop_stack(&s);       /* error */

    /* ---- очередь: жду 3 / 1 / 1 2 3 / error, потом reverse ---- */
    queue q = { NULL, NULL, 0 };
    printf("queue\n");
    enqueue(&q, 1);
    enqueue(&q, 2);
    enqueue(&q, 3);
    size_queue(&q);      /* 3 */
    front_queue(&q);     /* 1 */
    dequeue(&q);         /* 1 */
    dequeue(&q);         /* 2 */
    dequeue(&q);         /* 3 */
    dequeue(&q);         /* error */

    enqueue(&q, 1);
    enqueue(&q, 2);
    enqueue(&q, 3);
    reverse_queue(&q);
    dequeue(&q);         /* 3 */
    dequeue(&q);         /* 2 */
    dequeue(&q);         /* 1 */

    /* ---- дека: жду 1 3 2, потом reverse-демо ---- */
    deque d = { NULL, NULL, 0 };
    printf("deque\n");
    push_back_deque(&d, 2);
    push_back_deque(&d, 3);
    push_front_deque(&d, 1);   /* дека: 1 2 3 */
    pop_front_deque(&d);       /* 1 */
    pop_back_deque(&d);        /* 3 */
    pop_front_deque(&d);       /* 2 */
    pop_back_deque(&d);        /* error */

    push_back_deque(&d, 1);
    push_back_deque(&d, 2);
    push_back_deque(&d, 3);    /* дека: 1 2 3 */
    reverse_deque(&d);         /* дека: 3 2 1 */
    peek_front_deque(&d);      /* 3 */
    peek_back_deque(&d);       /* 1 */
    size_deque(&d);            /* 3 */

    return 0;
}
