#include <stdlib.h>
#include <stdio.h>
#include <arm_sve.h>

const size_t N = 1 << 28;

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

void run_scalar(size_t n, int *a, long *t);
void run_vector(size_t n, int *a, long *t);

int main(void)
{
    int *a;
    long t[3], freq;

    a = malloc(N * sizeof(int));
    if (!a)
        exit(EXIT_FAILURE);

    for (size_t i = 0; i < N; ++i) {
        a[i] = rand() % 10;
    }

    run_scalar(N, a, t);
    printf("Scalar\t%ld cycles\t%.2f elems/cycle\n", t[1] - t[0],
           (double) N / (t[1] - t[0]));

    run_vector(N, a, t);
    printf("SVE\t%ld cycles\t%.2f elems/cycle\n", t[2] - t[0],
           (double) N / (t[2] - t[0]));
    printf("cntw\t%ld cycles\n", t[1] - t[0]);
    printf("rem\t%ld cycles\n", t[2] - t[1]);

    freq = read_counter_frequency();
    printf("Freq\t%.2e Hz\n", (double) freq);

    free(a);

    return 0;
}
