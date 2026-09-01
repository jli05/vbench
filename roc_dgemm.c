#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <rocblas/rocblas.h>
#include <hip/hip_runtime.h>

#define CHECK_ROCBLAS(x)                                      \
    do {                                                       \
        rocblas_status status = (x);                          \
        if (status != rocblas_status_success) {               \
            fprintf(stderr, "rocBLAS error %d at %s:%d\n",   \
                    status, __FILE__, __LINE__);              \
            exit(EXIT_FAILURE);                               \
        }                                                      \
    } while (0)

#define CHECK_HIP(x)                                          \
    do {                                                       \
        hipError_t err = (x);                                 \
        if (err != hipSuccess) {                              \
            fprintf(stderr, "HIP error: %s at %s:%d\n",       \
                    hipGetErrorString(err), __FILE__, __LINE__); \
            exit(EXIT_FAILURE);                               \
        }                                                      \
    } while (0)

int main(int argc, char **argv)
{
    int N = 8000;
    int iters = 1;

    if (argc > 1) N = atoi(argv[1]);
    if (argc > 2) iters = atoi(argv[2]);

    size_t bytes = (size_t)N * N * sizeof(double);

    printf("N = %d, iterations = %d\n", N, iters);
    printf("Matrix memory: %.2f GiB each\n",
           (double)bytes / (1024.0 * 1024.0 * 1024.0));

    /* Host matrices */
    double *hA = malloc(bytes);
    double *hB = malloc(bytes);
    double *hC = malloc(bytes);

    if (!hA || !hB || !hC) {
        fprintf(stderr, "Host allocation failed\n");
        return 1;
    }

    /* Initialize */
    for (size_t i = 0; i < (size_t)N * N; i++) {
        hA[i] = 1.0;
        hB[i] = 2.0;
        hC[i] = 0.0;
    }

    /* Device matrices */
    void *dA, *dB, *dC;

    CHECK_HIP(hipMalloc(&dA, bytes));
    CHECK_HIP(hipMalloc(&dB, bytes));
    CHECK_HIP(hipMalloc(&dC, bytes));

    CHECK_HIP(hipMemcpy(dA, hA, bytes, hipMemcpyHostToDevice));
    CHECK_HIP(hipMemcpy(dB, hB, bytes, hipMemcpyHostToDevice));
    CHECK_HIP(hipMemcpy(dC, hC, bytes, hipMemcpyHostToDevice));

    /* rocBLAS handle */
    rocblas_handle handle;
    CHECK_ROCBLAS(rocblas_create_handle(&handle));

    double alpha = 1.0;
    double beta  = 0.0;

    /*
     * C = alpha*A*B + beta*C
     *
     * rocBLAS uses column-major matrices.
     */
    CHECK_ROCBLAS(
        rocblas_dgemm(
            handle,
            rocblas_operation_none,
            rocblas_operation_none,
            N, N, N,
            &alpha,
            dA, N,
            dB, N,
            &beta,
            dC, N
        )
    );

    /* Make sure the warm-up operation has finished */
    CHECK_HIP(hipDeviceSynchronize());

    /* GPU timing */
    hipEvent_t start, stop;
    CHECK_HIP(hipEventCreate(&start));
    CHECK_HIP(hipEventCreate(&stop));

    CHECK_HIP(hipEventRecord(start, 0));

    for (int i = 0; i < iters; i++) {
        CHECK_ROCBLAS(
            rocblas_dgemm(
                handle,
                rocblas_operation_none,
                rocblas_operation_none,
                N, N, N,
                &alpha,
                dA, N,
                dB, N,
                &beta,
                dC, N
            )
        );
    }

    CHECK_HIP(hipEventRecord(stop, 0));
    CHECK_HIP(hipEventSynchronize(stop));

    float ms;
    CHECK_HIP(hipEventElapsedTime(&ms, start, stop));

    double seconds = (ms / 1000.0) / iters;

    /*
     * DGEMM performs approximately 2*N^3 FLOPs.
     */
    double flops = (double)N * N * N;
    double gflops = flops / seconds / 1e9;

    printf("\nAverage GPU time: %.3f ms\n", seconds * 1000.0);
    printf("FP64 performance: %.2f GFLOP/s\n", gflops);
    printf("FP64 performance: %.2f TFLOP/s\n", gflops / 1000.0);

    /* Cleanup */
    CHECK_ROCBLAS(rocblas_destroy_handle(handle));

    CHECK_HIP(hipEventDestroy(start));
    CHECK_HIP(hipEventDestroy(stop));

    CHECK_HIP(hipFree(dA));
    CHECK_HIP(hipFree(dB));
    CHECK_HIP(hipFree(dC));

    free(hA);
    free(hB);
    free(hC);

    return 0;
}

