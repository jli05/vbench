#include <stdlib.h>
#include <err.h>
#include <stdio.h>
#include <math.h>

typedef float elem_type;

double now_seconds(void);

/*
 * Fortran BLAS interface.
 *
 * On most Unix-like systems, the symbol is sgemm_.
 */
void sgemm_(
    const char *transa,
    const char *transb,
    const int *m,
    const int *n,
    const int *k,
    const elem_type *alpha,
    const elem_type *a,
    const int *lda,
    const elem_type *b,
    const int *ldb,
    const elem_type *beta,
    elem_type *c,
    const int *ldc
);

/*
 * Column-major matrix initialization.
 */
void initialize_matrix(elem_type *x, const size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        x[i] = sinf((float) i) * .01f;
    }
}

double max_abs_error(
    const elem_type *a,
    const elem_type *b,
    const size_t count)
{
    double max_error = 0.0;

    for (size_t i = 0; i < count; ++i) {
        double error =
            fabs((double)a[i] - (double)b[i]);

        if (error > max_error)
            max_error = error;
    }

    return max_error;
}

/*
 * Reference SGEMM using the same column-major layout:
 *
 *     C = A * B
 *
 * A: M x K
 * B: K x N
 * C: M x N
 */
void reference_gemm(
    const size_t M,
    const size_t N,
    const size_t K,
    const elem_type *A,
    const elem_type *B,
    elem_type *C)
{
    elem_type sum;

    for (size_t j = 0; j < N; ++j) {
        for (size_t i = 0; i < M; ++i) {
            sum = 0.0;

            for (int k = 0; k < K; ++k) {
                sum +=
                    (double)A[i + k * M] *
                    (double)B[k + j * K];
            }

            C[i + j * M] = sum;
        }
    }
}


int main(void)
{
    const size_t M = 10000;
    const size_t N = 10000;
    const size_t K = 10000;

    double t[2];

    /*
     * Column-major BLAS layout:
     *
     * A = M x K
     * B = K x N
     * C = M x N
     */
    size_t size_A = M * K;
    size_t size_B = K * N;
    size_t size_C = M * N;

    elem_type *A, *B, *C, *C_ref;
    A = malloc(size_A * sizeof(elem_type));
    if (A == NULL)
        err(EXIT_FAILURE, "A malloc");
    B = malloc(size_B * sizeof(elem_type));
    if (B == NULL)
        err(EXIT_FAILURE, "B malloc");
    C = malloc(size_C * sizeof(elem_type));
    if (C == NULL)
        err(EXIT_FAILURE, "C malloc");
    C_ref = malloc(size_C * sizeof(elem_type));
    if (C_ref == NULL)
        err(EXIT_FAILURE, "C_ref malloc");

    initialize_matrix(A, size_A);
    initialize_matrix(B, size_B);

    /*
     * BLAS parameters.
     *
     * C = alpha * A * B + beta * C
     */
    const char transa = 'N';
    const char transb = 'N';

    const elem_type alpha = 1.0f;
    const elem_type beta  = 0.0f;

    const int lda = (int) M;
    const int ldb = (int) K;
    const int ldc = (int) M;

    int int_M, int_N, int_K;
    int_M = (int) M;
    int_N = (int) N;
    int_K = (int) K;

    t[0] = now_seconds();
    sgemm_(
        &transa,
        &transb,
        &int_M,
        &int_N,
        &int_K,
        &alpha,
        A,
        &lda,
        B,
        &ldb,
        &beta,
        C,
        &ldc
    );
    t[1] = now_seconds();

#ifdef CPU
    reference_gemm(M, N, K, A, B, C_ref);

    double error =
        max_abs_error(C, C_ref, size_C);

    printf("Maximum absolute error: %.6e\n", error);

    if (error > 1e-3) {
        fprintf(stderr,
                "ERROR: SGEMM result is incorrect\n");

        free(A);
        free(B);
        free(C);
        free(C_ref);

        return EXIT_FAILURE;
    }
#endif

    double flops =
        2.0 *
        (double)M *
        (double)N *
        (double)K;
    double gflops =
        flops / (t[1] - t[0]) / 1e9;
    printf("%.1f GFLOP/sec\n", gflops);

    free(A);
    free(B);
    free(C);
    free(C_ref);

    return EXIT_SUCCESS;
}
