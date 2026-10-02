#include <err.h>
#include <stdlib.h>
#include <stdio.h>
#include <arm_sve.h>

const size_t N = 1 << 25;
const size_t print_n = (N > 10)? 10 : N;

typedef int elem_type;
typedef unsigned long clock_counter_type;

static inline clock_counter_type read_counter_frequency(void)
{
    clock_counter_type freq;
    asm volatile("mrs %0, cntfrq_el0" : "=r"(freq));
    return freq;
}

void run_scalar(size_t n, const elem_type *a, long *t);
void run_vector(size_t n, const elem_type *a, long *t);

void print_int_vector(size_t n, const int *a);
void print_long_vector(size_t n, const long *a);

int main(void)
{
    elem_type *a;
    clock_counter_type t[3], freq;

    a = malloc(N * sizeof(elem_type));
    if (a == NULL)
	err(EXIT_FAILURE, "a malloc");

    for (size_t i = 0; i < N; ++i) {
        a[i] = rand();
    }

    printf("N\t%ld\n", N);

    printf("a: ");
    print_int_vector(print_n, a);

    run_scalar(N, a, t);
    printf("Scalar\t%lu cycles\t%.2f elems/cycle\n", t[1] - t[0],
           (double) N / (t[1] - t[0]));

    run_vector(N, a, t);
    printf("SVE\t%lu cycles\t%.2f elems/cycle\n", t[2] - t[0],
           (double) N / (t[2] - t[0]));
    printf("cnt_\t%lu cycles\n", t[1] - t[0]);
    printf("rem\t%lu cycles\n", t[2] - t[1]);

    freq = read_counter_frequency();
    printf("Freq\t%.2e Hz\n", (double) freq);

    free(a);

    return 0;
}
