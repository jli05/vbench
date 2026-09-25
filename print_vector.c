#include <stdio.h>

void print_int_vector(size_t n, const int *a)
{
    for (size_t i = 0; i < n; ++i)
        printf("%d ", a[i]);
    printf("\n");
}

void print_long_vector(size_t n, const long *a)
{
    for (size_t i = 0; i < n; ++i)
        printf("%ld ", a[i]);
    printf("\n");
}
