#include <stdlib.h>
#include <stdio.h>
#include <arm_sve.h>

const size_t N = 1 << 28;
const size_t half_N = N / 2;
const size_t print_n = (half_N > 10)? 10 : half_N;

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

void run_scalar(size_t n, int *a, int *b, int *result, long *t)
{
    t[0] = read_clock_counter();
    for (int i = 0; i < n; ++i) {
        result[i] = a[i] * b[i];
    }
    t[1] = read_clock_counter();
}

void run_vector(size_t n, int *a, int *b, int *result, long *t);

void print_int_vector(size_t n, int *a);

int main(void)
{
    int *a, *b, *result;
    long t[3], freq;

    a = malloc(half_N * sizeof(int));
    if (!a)
        exit(EXIT_FAILURE);
    b = malloc(half_N * sizeof(int));
    if (!b)
        exit(EXIT_FAILURE);
    result = malloc(half_N * sizeof(int));
    if (!result)
        exit(EXIT_FAILURE);

    for (size_t i = 0; i < half_N; ++i) {
        a[i] = rand() % 10;
        b[i] = rand() % 10;
    }
    printf("a: ");
    print_int_vector(print_n, a);
    printf("b: ");
    print_int_vector(print_n, b);

    run_scalar(half_N, a, b, result, t);
    printf("Scalar result: ");
    print_int_vector(print_n, result);

    printf("Scalar\t%ld cycles\t%.2f elems/cycle\n", t[1] - t[0],
           (double) N / (t[1] - t[0]));

    /*
     * SVE vectors have implementation-dependent lengths, so process
     * the 16 elements using svwhilelt_b32() to create a predicate for
     * the active elements.
     */
    run_vector(half_N, a, b, result, t);
    printf("Vector result: ");
    print_int_vector(print_n, result);

    printf("SVE\t%ld cycles\t%.2f elems/cycle\n", t[2] - t[0],
           (double) N / (t[2] - t[0]));
    printf("cntw\t%ld cycles\n", t[1] - t[0]);
    printf("rem\t%ld cycles\n", t[2] - t[1]);

    freq = read_counter_frequency();
    printf("Freq\t%.2e Hz\n", (double) freq);

    free(a);
    free(b);
    free(result);

    return 0;
}
