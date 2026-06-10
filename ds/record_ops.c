#include <stdio.h>
#include <string.h>

int main(void)
{
    struct person {
        char name[20];
        int age;
        float height;
    };

    struct person p1;
    strcpy(p1.name, "Ivan");
    p1.age = 25;
    p1.height = 1.75;

    struct person p2;
    p2 = p1;
    printf("%s %d %f\n", p2.name, p2.age, p2.height);

    if (strcmp(p1.name,p2.name) == 0 && p1.age == p2.age && p1.height == p2.height)
        printf("равны\n");

    struct pad {
        char c;
        int q;
    };
    struct pad p;
    printf("%lu", sizeof(p));
    return 0;
}