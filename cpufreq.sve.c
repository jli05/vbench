#include <stdlib.h>
#include <stdio.h>

typedef unsigned long clock_counter_type;

int main(void)
{
    clock_counter_type freq;
    asm volatile("mrs %0, cntfrq_el0" : "=r"(freq));
    printf("%lu MHz\n", freq / 1000000);

    return EXIT_SUCCESS;
}
