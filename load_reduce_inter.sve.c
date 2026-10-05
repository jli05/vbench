#include <stdlib.h>
#include <err.h>
#include <stdio.h>
#include <arm_sve.h>

const size_t N = 1 << 25;

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

elem_type run_scalar(size_t n, const elem_type *a,
                     clock_counter_type *t)
{
    t[0] = read_clock_counter();
    elem_type sum = 0;
    for (size_t i = 0; i < n; ++i) {
        sum += a[i];
    }
    t[1] = read_clock_counter();
    return sum;
}

elem_type run_vector(size_t n, const elem_type *a,
                     clock_counter_type *t);

int main(void)
{
    elem_type *a, b, c;
    clock_counter_type t[3], freq;

    a = malloc(N * sizeof(elem_type));
    if (a == NULL)
        err(EXIT_FAILURE, "a malloc");

    for (size_t i = 0; i < N; ++i) {
        a[i] = rand();
    }

    b = run_scalar(N, a, t);
    printf("Scalar\t%lu cycles\t%.2f elems/cycle\n", t[1] - t[0],
           (double) N / (t[1] - t[0]));

    c = run_vector(N, a, t);
    printf("SVE\t%lu cycles\t%.2f elems/cycle\n", t[2] - t[0],
           (double) N / (t[2] - t[0]));
    printf("cnt_\t%lu cycles\n", t[1] - t[0]);
    printf("rem\t%lu cycles\n", t[2] - t[1]);

    if (b != c)
        fprintf(stderr, "b != c\n");

    freq = read_counter_frequency();
    printf("Freq\t%.2e Hz\n", (double) freq);

    free(a);

    return 0;
}
