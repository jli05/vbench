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
    _mm_mfence();
    uint64_t t = __rdtsc();
    _mm_lfence();
    return t;
}

static inline uint64_t rdtsc_stop(void)
{
    uint64_t t;

    _mm_lfence();
    t = __rdtsc();
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

static uint64_t bench_scalar(const uint32_t *data,
                             size_t n)
{
    uint64_t start = rdtsc_start();

    size_t half_n = n / 2;
    const int * b = &data[half_n];

    for (int it = 0; it < ITERS; ++it) {
        for (size_t i = 0; i < half_n; ++i)
            consume_u32(data[i] * b[i]);
    }

    uint64_t end = rdtsc_stop();

    return end - start;
}

static uint64_t bench_vector(const uint32_t *data,
                             size_t n)
{
    uint64_t start = rdtsc_start();

    size_t half_n = n / 2;
    const int * b = &data[half_n];

    __m512i va, vb, res;

    for (int it = 0; it < ITERS; ++it) {
        for (size_t i = 0; i < half_n; i += 16) {
            va = _mm512_loadu_si512(&data[i]);
            vb = _mm512_loadu_si512(&b[i]);
            res = _mm512_mullo_epi32(va, vb);

            consume_vector(res);
        }
    }

    uint64_t end = rdtsc_stop();

    return end - start;
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

    const size_t data_elems = (size_t)N;

    uint32_t *data = aligned_alloc(
        64,
        data_elems * sizeof(*data)
    );

    if (!data) {
        perror("aligned_alloc");
        return 1;
    }

    for (size_t i = 0; i < data_elems; ++i)
        data[i] = (uint32_t)rand();

    uint64_t cycles = bench_vector(data, N);
    uint64_t cycles0 = bench_scalar(data, N);

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

    free(data);

    return 0;
}
