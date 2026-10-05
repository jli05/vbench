#include <stdlib.h>
#include <err.h>
#include <stdio.h>
#include <immintrin.h>

const size_t N = 1 << 25;
const size_t print_n = (N > 10)? 10 : N;

typedef int elem_type;
typedef unsigned long long clock_counter_type;

const size_t n_elems = 512 / (sizeof(elem_type) << 3);

static inline clock_counter_type rdtsc(void)
{
    _mm_mfence();
    clock_counter_type t = __rdtsc();
    _mm_mfence();
    return t;
}

static inline void consume_scalar(elem_type x)
{
    asm volatile("" : : "r"(x) : "memory");
}

static inline void consume_vector(__m512i v)
{
    /*
     * Compiler barrier: v must actually be produced,
     * but no arithmetic is performed on it.
     */
    asm volatile("" : : "v"(v) : "memory");
}

elem_type run_scalar(size_t n, const elem_type *data,
                     clock_counter_type *t)
{
    t[0] = rdtsc();
    elem_type sum = 0;
    for (size_t i = 0; i < n; ++i)
        sum += data[i];
    t[1] = rdtsc();
    return sum;
}

elem_type run_vector(size_t n, const elem_type *data,
                      clock_counter_type *t)
{
    t[0] = rdtsc();

    elem_type sum = 0;
    __m512i vi;

    for (size_t i = 0; i < n; i += n_elems) {
        vi = _mm512_loadu_epi32(&data[i]);
        sum += _mm512_reduce_add_epi32(vi);
    }

    t[1] = rdtsc();

    return sum;
}

int main(int argc, char **argv)
{
    elem_type *a, sum1, sum2;

    a = malloc(N * sizeof(elem_type));
    if (a == NULL)
        err(EXIT_FAILURE, "a alloc");

    for (size_t i = 0; i < N; ++i)
        a[i] = rand();

    clock_counter_type t[2];

    sum1 = run_scalar(N, a, t);
    printf("Scalar\t%llu cycles\t%.2f elems/cycle\n",
           t[1] - t[0], (double) N / (t[1] - t[0]));

    sum2 = run_vector(N, a, t);
    printf("Vector\t%llu cycles\t%.2f elems/cycle\n",
           t[1] - t[0], (double) N / (t[1] - t[0]));

    if (sum1 != sum2)
        fprintf(stderr, "scalar sum = %ld, vector sum = %ld\n",
                sum1, sum2);

    free(a);

    return 0;
}
