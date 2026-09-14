#define _GNU_SOURCE
#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>

#ifndef N
#define N (1 << 24)
#endif

#ifndef ITERS
#define ITERS 10
#endif

static inline uint64_t rdtsc_start(void)
{
    unsigned aux;
    _mm_mfence();
    uint64_t t = __rdtscp(&aux);
    _mm_lfence();
    return t;
}

static inline uint64_t rdtsc_stop(void)
{
    unsigned aux;
    uint64_t t;

    _mm_lfence();
    t = __rdtscp(&aux);
    _mm_mfence();

    return t;
}

static void pin_cpu(int cpu)
{
    cpu_set_t set;

    CPU_ZERO(&set);
    CPU_SET(cpu, &set);

    if (sched_setaffinity(0, sizeof(set), &set) != 0)
        perror("sched_setaffinity");
}

static inline void consume_u32(uint32_t x)
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

static uint64_t bench_scalar(const int32_t *idx,
                             const uint32_t *data,
                             size_t n)
{
    uint64_t start = rdtsc_start();

    for (int it = 0; it < ITERS; ++it) {
        for (size_t i = 0; i < n; ++i)
            consume_u32(data[idx[i]]);
    }

    uint64_t end = rdtsc_stop();

    return end - start;
}

static uint64_t bench_gather(const int32_t *idx,
                             const uint32_t *data,
                             size_t n)
{
    uint64_t start = rdtsc_start();

    __m512i vi, v;

    for (int it = 0; it < ITERS; ++it) {
        for (size_t i = 0; i < n; i += 16) {
            vi = _mm512_loadu_si512(&idx[i]);

            v = _mm512_i32gather_epi32(
                vi,
                data,
                4
            );

            consume_vector(v);
        }
    }

    uint64_t end = rdtsc_stop();

    return end - start;
}

static void make_indices(int32_t *idx,
                          size_t n,
                          size_t data_elems)
{
    uint32_t x = 0x12345678;

    for (size_t i = 0; i < n; ++i) {
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;

        idx[i] = (int32_t)(x % data_elems);
    }
}

int main(int argc, char **argv)
{
    int cpu = 0;

    if (argc > 1)
        cpu = atoi(argv[1]);

    printf("AVX-512 gather benchmark\n");
    printf("N       = %zu\n", (size_t)N);
    printf("ITERS   = %d\n", ITERS);
    printf("CPU     = %d\n", cpu);

    pin_cpu(cpu);

    const size_t data_elems = (size_t)N * 4;

    int32_t *idx = aligned_alloc(64, N * sizeof(*idx));
    uint32_t *data = aligned_alloc(
        64,
        data_elems * sizeof(*data)
    );

    if (!idx || !data) {
        perror("aligned_alloc");
        return 1;
    }

    for (size_t i = 0; i < data_elems; ++i)
        data[i] = (uint32_t)i;

    make_indices(idx, N, data_elems);

    uint64_t cycles = bench_gather(idx, data, N);
    uint64_t cycles0 = bench_scalar(idx, data, N);


    double cpe =
        (double)cycles / ((double)N * ITERS);

    printf("\nResults\n");
    printf("-------\n");
    printf("scalar\n");
    printf("cycles       : %llu\n",
           (unsigned long long)cycles0);
    printf("cycles/elem  : %.3f\n",
           (double) cycles0 / ((double) N * ITERS));

    printf("\nvector\n");
    printf("cycles       : %llu\n",
           (unsigned long long)cycles);
    printf("cycles/elem  : %.3f\n",
           (double) cycles / ((double) N * ITERS));

    free(idx);
    free(data);

    return 0;
}
