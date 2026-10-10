#include <stdlib.h>
#include <err.h>
#include <stdio.h>
#include <arm_sve.h>

const size_t N = 1 << 25;

typedef int elem_type;
typedef unsigned long clock_counter_type;

double now_seconds(void);

static inline clock_counter_type read_clock_counter()
{
    clock_counter_type counter;
    asm volatile("isb\n"
                 "mrs %0, cntvct_el0\n"
		 "isb\n"
                 : "=r"(counter));
    return counter;
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
    clock_counter_type t[3];
    double u[2];

    a = malloc(N * sizeof(elem_type));
    if (a == NULL)
        err(EXIT_FAILURE, "a malloc");

    for (size_t i = 0; i < N; ++i) {
        a[i] = rand();
    }

    u[0] = now_seconds();
    b = run_scalar(N, a, t);
    u[1] = now_seconds();
    printf("Scalar\t%.1fG elems/sec\t%.2f elems/cycle\n",
           (double) N / (u[1] - u[0]) / 1e9, (double) N / (t[1] - t[0]));

    u[0] = now_seconds();
    c = run_vector(N, a, t);
    u[1] = now_seconds();
    printf("SVE\t%.1fG elems/sec\t%.2f elems/cycle\n",
           (double) N / (u[1] - u[0]) / 1e9, (double) N / (t[2] - t[0]));
    printf("cnt_\t%lu cycles\n", t[1] - t[0]);
    printf("rem\t%lu cycles\n", t[2] - t[1]);

    if (b != c)
        fprintf(stderr, "b != c\n");

    free(a);

    return 0;
}
