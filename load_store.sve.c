#include <err.h>
#include <stdlib.h>
#include <stdio.h>
#include <arm_sve.h>

const size_t N = 1 << 25;
const size_t print_n = (N > 10)? 10 : N;

typedef int elem_type;
typedef unsigned long clock_counter_type;

static inline clock_counter_type read_clock_counter()
{
    clock_counter_type counter;
    asm volatile("isb\n"
                 "mrs %0, cntvct_el0\n"
		 "isb\n"
                 : "=r"(counter));
    return counter;
}

static inline clock_counter_type read_counter_frequency(void)
{
    clock_counter_type freq;
    asm volatile("mrs %0, cntfrq_el0" : "=r"(freq));
    return freq;
}

void run_scalar(size_t n, const elem_type *a, elem_type *result,
                clock_counter_type *t)
{
    t[0] = read_clock_counter();
    for (size_t i = 0; i < n; ++i)
        result[i] = a[i];
    t[1] = read_clock_counter();
}

void print_int_vector(size_t n, const int *a);
void print_long_vector(size_t n, const long *a);

void run_vector(size_t n, const elem_type *a, elem_type *result,
                clock_counter_type *t);

int main(void)
{
    elem_type *a, *b, *c;
    clock_counter_type t[3], freq;

    a = malloc(N * sizeof(elem_type));
    if (a == NULL)
	err(EXIT_FAILURE, "a malloc");
    b = malloc(N * sizeof(elem_type));
    if (b == NULL)
        err(EXIT_FAILURE, "b malloc");
    c = malloc(N * sizeof(elem_type));
    if (c == NULL)
        err(EXIT_FAILURE, "c malloc");

    for (size_t i = 0; i < N; ++i) {
        a[i] = rand();
    }

    printf("N\t%ld\n", N);

    printf("a: ");
    print_int_vector(print_n, a);

    run_scalar(N, a, b, t);
    printf("Scalar result: ");
    print_int_vector(print_n, b);
    printf("Scalar\t%lu cycles\t%.2f elems/cycle\n", t[1] - t[0],
           (double) N / (t[1] - t[0]));

    run_vector(N, a, c, t);
    printf("Vector result: ");
    print_int_vector(print_n, c);
    printf("SVE\t%lu cycles\t%.2f elems/cycle\n", t[2] - t[0],
           (double) N / (t[2] - t[0]));
    printf("cnt_\t%lu cycles\n", t[1] - t[0]);
    printf("rem\t%lu cycles\n", t[2] - t[1]);

    for (size_t i = 0; i < N; ++i)
        if (b[i] != c[i])
            fprintf(stderr, "b[%lu] = %d, c[%lu] = %d\n", i, b[i], i, c[i]);

    freq = read_counter_frequency();
    printf("Freq\t%.2e Hz\n", (double) freq);

    free(a);
    free(b);
    free(c);

    return 0;
}
