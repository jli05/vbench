#include <stdio.h>

void print_int_vector(size_t n, const int *a)
{
    for (size_t i = 0; i < n; ++i)
        printf("%d ", a[i]);
    printf("\n");
}
