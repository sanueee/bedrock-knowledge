#include <stddef.h>
#include <stdio.h>

#define K (size_t)10

int main(void)
{
    int a[] = {3,1,4,1,5,9,2,6,5,3,5};
    int freq[K] = {0};
    
    size_t len = sizeof(a) / sizeof(a[0]);
    for (size_t i = 0; i < len; i++)
        freq[a[i]]++;

    /*
    for (size_t i = 0; i < K; i++)
        printf("%d ", freq[i]);
    */

    int mx = 0; int max_int = 0;
    for (size_t i = 0; i < K; i++)
    {
        if (freq[i] > mx)
        {
            mx = freq[i];
            max_int = i;
        }
    }

    printf("\n%d", max_int);

    return 0;
}