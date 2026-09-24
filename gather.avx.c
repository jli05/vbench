#include <immintrin.h>
#include <stdio.h>
#include <stdlib.h>

const size_t N = 1 << 18;
const size_t M = 4 * N;

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
                  const size_t *indices,
                  unsigned long long *t)
{
    t[0] = rdtsc();
    for (size_t i = 0; i < n; ++i)
        consume_u32(data[indices[i]]);
    t[1] = rdtsc();
}

void bench_vector(size_t n, const int *data,
                  const size_t *indices,
                  unsigned long long *t)
{
    t[0] = rdtsc();

    __m512i vi;
    __m256i v;

    for (size_t i = 0; i < n; i += 8) {
        vi = _mm512_loadu_si512(&indices[i]);
        v = _mm512_i64gather_epi32(vi, data, 4);
        asm volatile("" : : "v"(v) : "memory");
    }

    t[1] = rdtsc();
}

static void make_indices(size_t *idx,
                         size_t n,
                         size_t data_elems)
{
    size_t x = 0x12345678;

    for (size_t i = 0; i < n; ++i) {
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;

        idx[i] = x % data_elems;
    }
}

int main(int argc, char **argv)
{
    int *data = malloc(M * sizeof(int));
    size_t *indices = malloc(N * sizeof(size_t));
    if (!data)
        exit(EXIT_FAILURE);
    if (!indices)
        exit(EXIT_FAILURE);

    for (size_t i = 0; i < N; ++i)
        data[i] = rand();
    make_indices(indices, N, M);

    unsigned long long t[2];

    bench_scalar(N, data, indices, t);
    printf("Scalar\t%llu cycles\t%.2f elems/cycle\n",
           t[1] - t[0], (double) N / (t[1] - t[0]));

    bench_vector(N, data, indices, t);
    printf("Vector\t%llu cycles\t%.2f elems/cycle\n",
           t[1] - t[0], (double) N / (t[1] - t[0]));

    free(data);

    return 0;
}
