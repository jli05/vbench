#include <stdlib.h>
#include <err.h>
#include <math.h>
#include <stdio.h>
#include <cuda_runtime.h>
#include <cublas_v2.h>

typedef float elem_type;

#define CUDA_CHECK(call)                                      \
    do {                                                      \
        cudaError_t err = (call);                             \
        if (err != cudaSuccess) {                             \
            fprintf(stderr,                                  \
                    "CUDA error at %s:%d: %s\n",              \
                    __FILE__, __LINE__,                       \
                    cudaGetErrorString(err));                 \
            exit(EXIT_FAILURE);                               \
        }                                                     \
    } while (0)


#define CUBLAS_CHECK(call)                                    \
    do {                                                      \
        cublasStatus_t status = (call);                       \
        if (status != CUBLAS_STATUS_SUCCESS) {               \
            fprintf(stderr,                                  \
                    "cuBLAS error at %s:%d: status=%d\n",     \
                    __FILE__, __LINE__, (int)status);          \
            exit(EXIT_FAILURE);                               \
        }                                                     \
    } while (0)


static void
init_matrix(elem_type *A, const size_t n)
{
    for (size_t i = 0; i < n; i++) {
        A[i] = sinf((elem_type)i) * 0.01f;
    }
}


static void
cpu_sgemm(
    const size_t M,
    const size_t N,
    const size_t K,
    const elem_type *A,
    const elem_type *B,
    elem_type *C)
{
    /*
     * Column-major matrices:
     *
     * A: M x K
     * B: K x N
     * C: M x N
     *
     * C = A * B
     */

    for (size_t i = 0; i < M; i++) {
        for (size_t j = 0; j < N; j++) {
	    C[i + j * M] = 0.0;
            for (size_t k = 0; k < K; k++) {
                C[i + j * M] +=
                    A[i + k * M] *
                    B[k + j * K];
            }
        }
    }
}


static double
max_error(
    const elem_type *A,
    const elem_type *B,
    const size_t n)
{
    double max_err = 0.0;

    for (size_t i = 0; i < n; i++) {
        double err = fabs((double)A[i] - (double)B[i]);

        if (err > max_err)
            max_err = err;
    }

    return max_err;
}


int
main(void)
{
    const size_t M = 35000;
    const size_t N = 35000;
    const size_t K = 35000;

    size_t size_A = M * K;
    size_t size_B = K * N;
    size_t size_C = M * N;

    size_t bytes_A = size_A * sizeof(elem_type);
    size_t bytes_B = size_B * sizeof(elem_type);
    size_t bytes_C = size_C * sizeof(elem_type);

    elem_type *h_A;
    elem_type *h_B;
    elem_type *h_C;
    elem_type *h_C_ref;

    elem_type *d_A;
    elem_type *d_B;
    elem_type *d_C;

    cublasHandle_t handle;

    cudaEvent_t start;
    cudaEvent_t stop;

    float elapsed_ms;
    float avg_ms;

    double flops;
    double gflops;
    double tflops;
    double error;

    /*
     * Host allocations.
     */

    h_A = malloc(bytes_A);
    h_B = malloc(bytes_B);
    h_C = malloc(bytes_C);
    h_C_ref = malloc(bytes_C);

    if (h_A == NULL ||
        h_B == NULL ||
        h_C == NULL ||
        h_C_ref == NULL) {
        err(EXIT_FAILURE, "Host allocation failed\n");
    }

    /*
     * Initialize input matrices.
     */

    init_matrix(h_A, size_A);
    init_matrix(h_B, size_B);

    CUDA_CHECK(cudaEventCreate(&start));
    CUDA_CHECK(cudaEventCreate(&stop));

    /*
     * Device allocations.
     */

    CUDA_CHECK(cudaMalloc(
        (void **)&d_A,
        bytes_A));

    CUDA_CHECK(cudaMalloc(
        (void **)&d_B,
        bytes_B));

    CUDA_CHECK(cudaMalloc(
        (void **)&d_C,
        bytes_C));

    /*
     * Copy A and B to the GPU.
     */

    CUDA_CHECK(cudaEventRecord(start, 0));

    CUDA_CHECK(cudaMemcpy(
        d_A,
        h_A,
        bytes_A,
        cudaMemcpyHostToDevice));

    CUDA_CHECK(cudaMemcpy(
        d_B,
        h_B,
        bytes_B,
        cudaMemcpyHostToDevice));

    /*
     * Create cuBLAS context.
     */

    CUBLAS_CHECK(cublasCreate(&handle));

    /*
     * C = alpha * A * B + beta * C
     */

    /*
     * Benchmark.
     */

    {
        const elem_type alpha = 1.0f;
        const elem_type beta = 0.0f;


        CUBLAS_CHECK(cublasSgemm(
            handle,

            CUBLAS_OP_N,
            CUBLAS_OP_N,

            (int) M,
            (int) N,
            (int) K,

            &alpha,

            d_A,
            (int) M,

            d_B,
            (int) K,

            &beta,

            d_C,
            (int) M));
    }

    CUDA_CHECK(cudaMemcpy(
        h_C,
        d_C,
        bytes_C,
        cudaMemcpyDeviceToHost));

    CUDA_CHECK(cudaEventRecord(stop, 0));
    CUDA_CHECK(cudaEventSynchronize(stop));

    CUDA_CHECK(cudaEventElapsedTime(
        &elapsed_ms,
        start,
        stop));

    avg_ms = elapsed_ms;

    /*
     * SGEMM performs approximately:
     *
     *     2 * M * N * K
     *
     * floating-point operations.
     */

    flops =
        2.0 *
        (double)M *
        (double)N *
        (double)K;

    /*
     * Convert:
     *
     * FLOP / milliseconds
     * -------------------
     *      1e6
     *
     * gives GFLOP/s.
     */

    gflops = flops / ((double)avg_ms * 1.0e6);
    tflops = gflops / 1000.0;

    printf("Average time   : %.3f ms\n",
           avg_ms);

    printf("Performance    : %.2f GFLOP/s\n",
           gflops);

    printf("Performance    : %.2f TFLOP/s\n",
           tflops);

    /*
     * Verify the result using CPU SGEMM.
     */

    /*
     * For very large matrices this CPU check can be expensive.
     * It is mainly intended for correctness testing.
     */

#ifdef CPU
    cpu_sgemm(
        M,
        N,
        K,
        h_A,
        h_B,
        h_C_ref);

    error = max_error(
        h_C,
        h_C_ref,
        (int)size_C);

    printf("Maximum absolute error: %.6e\n",
           error);

    if (error < 1.0e-2) {
        printf("Result: PASS\n");
    } else {
        printf("Result: FAIL\n");
    }
#endif

    /*
     * Cleanup.
     */

    CUBLAS_CHECK(cublasDestroy(handle));

    CUDA_CHECK(cudaFree(d_A));
    CUDA_CHECK(cudaFree(d_B));
    CUDA_CHECK(cudaFree(d_C));

    CUDA_CHECK(cudaEventDestroy(start));
    CUDA_CHECK(cudaEventDestroy(stop));

    free(h_A);
    free(h_B);
    free(h_C);
    free(h_C_ref);

    return EXIT_SUCCESS;
}
