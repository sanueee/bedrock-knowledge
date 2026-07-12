#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int *data;
    size_t size;
    size_t cap;
} vec;

void vec_init(vec *v)
{
    v->data = NULL;
    v->cap = 0;
    v->size = 0;
}

void vec_push(vec *v, int x)
{
    if (v->size == v->cap)
    {
        size_t newcap = v->cap ? v->cap * 2 : 1;
        int *tmp = realloc((v->data), sizeof(int) * newcap);
        if (tmp == NULL) { perror("realloc"); return; }
        v->data = tmp;
        v->cap = newcap;
    }
    v->data[v->size++] = x;    
}

void vec_free(vec *v)
{
    free(v->data);
    v->data = NULL;
    v->cap = 0;
    v->size = 0;
}

int main(void)
{
    vec v;
    vec_init(&v);
    for (size_t i = 1; i <= 20; i++)
    {
        size_t capacity = v.cap;
        vec_push(&v, i);
        if (capacity != v.cap)
            printf("old capacity=%zu -> new capacity=%zu\n", capacity, v.cap);
    }
    vec_free(&v);
    
    return 0;
}