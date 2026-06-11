#include <stdio.h>
#include <string.h>

enum vtype { INT_T, FLOAT_T, STR_T };

struct variant {
    enum vtype tag;
    union { int i; float f; char str[20]; };
};

void print_variant(struct variant v)
{
    switch (v.tag) {
        case INT_T: {
            printf("%d\n", v.i);
            break;
        }
        case FLOAT_T: {
            printf("%f\n", v.f);
            break;
        }
        case STR_T: {
            printf("%s\n", v.str);
            break;
        }
    }
}

int main(void)
{
    struct variant data_i;
    data_i.tag = INT_T;
    data_i.i = 5;

    struct variant data_f;
    data_f.tag = FLOAT_T;
    data_f.f = 5.5;

    struct variant data_str;
    data_str.tag = STR_T;
    strcpy(data_str.str, "hi");

    print_variant(data_i);
    print_variant(data_f);
    print_variant(data_str);

    return 0;
}