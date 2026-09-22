#include <immintrin.h>
#include <stdio.h>
#include <stdlib.h>

const size_t N = 1 << 18;

static inline unsigned long long rdtsc(void)
{
    _mm_mfence();
    unsigned long long t = __rdtsc();
    _mm_mfence();
    return t;
}

static inline void consume_u32(int x)
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

void bench_scalar(size_t n, const int *data,
                  unsigned long long *t)
{
    t[0] = rdtsc();
    for (size_t i = 0; i < n; ++i)
        consume_u32(data[i]);
    t[1] = rdtsc();
}

void bench_vector(size_t n, const int *data,
                  unsigned long long *t)
{
    t[0] = rdtsc();

    __m512i vi;

    for (size_t i = 0; i < n; i += 16) {
        vi = _mm512_loadu_si512(&data[i]);
        consume_vector(vi);
    }

    t[1] = rdtsc();
}

int main(int argc, char **argv)
{
    int *data = malloc(N * sizeof(int));
    if (!data)
        exit(EXIT_FAILURE);

    for (size_t i = 0; i < N; ++i)
        data[i] = rand();

    unsigned long long t[2];

    bench_scalar(N, data, t);
    printf("Scalar\t%llu cycles\t%.2f elems/cycle\n",
           t[1] - t[0], (double) N / (t[1] - t[0]));

    bench_vector(N, data, t);
    printf("Vector\t%llu cycles\t%.2f elems/cycle\n",
           t[1] - t[0], (double) N / (t[1] - t[0]));

    free(data);

    return 0;
}
