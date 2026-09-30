#include <err.h>
#include <stdlib.h>
#include <stdio.h>
#include <arm_sve.h>

const size_t N = 1 << 25;
const size_t quad_N = 4 * N;
const size_t print_n = (N > 10)? 10 : N;

typedef int elem_type;

static inline long read_clock_counter()
{
    long counter;
    asm volatile("isb\n"
                 "mrs %0, cntvct_el0\n"
		 "isb\n"
                 : "=r"(counter));
    return counter;
}

static inline long read_counter_frequency(void)
{
    long freq;
    asm volatile("mrs %0, cntfrq_el0" : "=r"(freq));
    return freq;
}

static void make_indices(size_t *idx,
                         const size_t n,
                         const size_t data_elems)
{
    size_t x = 0x1234567886849210;

    for (size_t i = 0; i < n; ++i) {
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;

        idx[i] = x % data_elems;
    }
}


void run_scalar(size_t n, const elem_type *a, const size_t *indices,
                elem_type *result, long *t)
{
    t[0] = read_clock_counter();
    for (size_t i = 0; i < n; ++i)
        result[i] = a[indices[i]];
    t[1] = read_clock_counter();
}

void print_int_vector(size_t n, const int *a);
void print_long_vector(size_t n, const long *a);

void run_vector(size_t n, const elem_type *a, const size_t* indices,
                elem_type *result, long *t, size_t *indices2);

int main(void)
{
    elem_type *a, *b, *c;
    size_t *indices, *indices2;
    long t[3], freq;

    a = malloc(quad_N * sizeof(elem_type));
    if (a == NULL)
	err(EXIT_FAILURE, "a malloc");
    indices = malloc(N * sizeof(size_t));
    if (indices == NULL)
        err(EXIT_FAILURE, "indices malloc");
    indices2 = malloc(N * sizeof(size_t));
    if (indices2 == NULL)
        err(EXIT_FAILURE, "indices2");

    b = malloc(N * sizeof(elem_type));
    if (b == NULL)
        err(EXIT_FAILURE, "b malloc");
    c = malloc(N * sizeof(elem_type));
    if (c == NULL)
        err(EXIT_FAILURE, "c malloc");

    for (size_t i = 0; i < quad_N; ++i) {
        // a[i] = rand() % 10;
        a[i] = (int) i;
    }
    make_indices(indices, N, quad_N);

    printf("N\t%ld\n", N);

    printf("a: ");
    print_int_vector((quad_N > 40)? 40 : quad_N, a);
   // printf("indices: ");
   // for (size_t i = 0; i < N; ++i)
   //     printf("%lu ", indices[i]);
   //  printf("\n");

    run_scalar(N, a, indices, b, t);
    printf("Scalar result: ");
    print_int_vector(print_n, b);
    printf("Scalar\t%ld cycles\t%.2f elems/cycle\n", t[1] - t[0],
           (double) N / (t[1] - t[0]));

    run_vector(N, a, indices, c, t, indices2);
    printf("Vector result: ");
    print_int_vector(print_n, c);
    printf("SVE\t%ld cycles\t%.2f elems/cycle\n", t[2] - t[0],
           (double) N / (t[2] - t[0]));
    printf("cnt_\t%ld cycles\n", t[1] - t[0]);
    printf("rem\t%ld cycles\n", t[2] - t[1]);

   // for (size_t i = 0; i < N; ++i)
   //      printf("%lu ", indices2[i]);
   // printf("\n");
    for (size_t i = 0; i < N; ++i)
        if (b[i] != c[i])
            printf("b[%lu] = %d, c[%lu] = %d\n", i, b[i], i, c[i]);

    freq = read_counter_frequency();
    printf("Freq\t%.2e Hz\n", (double) freq);

    free(a);
    free(b);
    free(c);

    return 0;
}
