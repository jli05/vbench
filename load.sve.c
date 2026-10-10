#include <err.h>
#include <stdlib.h>
#include <stdio.h>
#include <arm_sve.h>

const size_t N = 1 << 25;

typedef int elem_type;
typedef unsigned long clock_counter_type;

double now_seconds(void);

void run_scalar(size_t n, const elem_type *a, long *t);
void run_vector(size_t n, const elem_type *a, long *t);

void print_int_vector(size_t n, const int *a);
void print_long_vector(size_t n, const long *a);

int main(void)
{
    elem_type *a;
    clock_counter_type t[3];
    double u[2];

    a = malloc(N * sizeof(elem_type));
    if (a == NULL)
	err(EXIT_FAILURE, "a malloc");

    for (size_t i = 0; i < N; ++i) {
        a[i] = rand();
    }

    u[0] = now_seconds();
    run_scalar(N, a, t);
    u[1] = now_seconds();
    printf("Scalar\t%.1fG elems/sec\t%.2f elems/cycle\n",
           (double) N / (u[1] - u[0]) / 1e9, (double) N / (t[1] - t[0]));

    u[0] = now_seconds();
    run_vector(N, a, t);
    u[1] = now_seconds();
    printf("SVE\t%.1fG elems/sec\t%.2f elems/cycle\n",
           (double) N / (u[1] - u[0]) / 1e9, (double) N / (t[2] - t[0]));
    printf("cnt_\t%lu cycles\n", t[1] - t[0]);
    printf("rem\t%lu cycles\n", t[2] - t[1]);

    free(a);

    return 0;
}
