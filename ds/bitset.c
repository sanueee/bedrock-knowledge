#include <stdio.h>
#include <stdint.h>

#define LEN 64

void set_add(uint64_t *s, int x)
{
    *s |= (uint64_t)1 << x;
}

void set_remove(uint64_t *s, int x)
{
    *s &= ~((uint64_t)1 << x);
}

int set_has(uint64_t s, int x)
{
    return ((s & ((uint64_t)1 << x)) != 0);
}

void set_print(uint64_t s)
{
    for (size_t i = 0; i < LEN; i++)
    {
        if (s & ((uint64_t)1 << i))
            printf("%zu ", i);
    }
    printf("\n");
}

int main(void)
{
    uint64_t A = 0;
    set_add(&A, 1);
    set_add(&A, 3);
    set_add(&A, 5);
    set_add(&A, 9);

    uint64_t B = 0;
    set_add(&B, 3);
    set_add(&B, 9);
    set_add(&B, 12);

    set_print(A | B);
    set_print(A & B);
    set_print(A & ~B);
    set_print(A ^ B);
    
    return 0;
}