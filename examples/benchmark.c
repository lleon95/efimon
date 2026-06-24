/**
 * @file benchmark.c
 * @author Diego Avila <diego.avila@uned.cr>
 *         Anthony Montero <anthonymr2010@estudiantec.cr>
 * @brief Benchmark program for CPU consumption testing
 *
 * @copyright Copyright (c) 2026. See License for Licensing
 */

#define _POSIX_C_SOURCE 200809L

#include <cblas.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static double now_sec(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec + ts.tv_nsec * 1e-9;
}

int main(int argc, char **argv) {
  if (argc < 4) {
    fprintf(stderr, "Usage: %s <N> <reps> <warmup>\n", argv[0]);
    return 1;
  }
  int N = atoi(argv[1]);      /* matrix size: N x N */
  int reps = atoi(argv[2]);   /* number of repetitions to average */
  int warmup = atoi(argv[3]); /* warmup runs before measured reps */

  /* allocate matrices (row-major for cblas, but using column-major calls
   * consistent with BLAS)
   */
  size_t elems = (size_t)N * (size_t)N;
  double *A = aligned_alloc(64, elems * sizeof(double));
  double *B = aligned_alloc(64, elems * sizeof(double));
  double *C = aligned_alloc(64, elems * sizeof(double));
  if (!A || !B || !C) {
    fprintf(stderr, "Allocation failed\n");
    return 1;
  }

  /* init with random values (deterministic seed) */
  srand(42);
  for (size_t i = 0; i < elems; ++i) {
    A[i] = ((double)rand() / RAND_MAX);
    B[i] = ((double)rand() / RAND_MAX);
    C[i] = 0.0;
  }

  /* Warmup runs (not measured) */
  for (int w = 0; w < warmup; ++w) {
    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, N, N, N, 1.0, A, N,
                B, N, 0.0, C, N);
  }

  /* Measured reps */
  double t0 = now_sec();
  for (int r = 0; r < reps; ++r) {
    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, N, N, N, 1.0, A, N,
                B, N, 0.0, C, N);
  }
  double t1 = now_sec();

  double elapsed = (t1 - t0); /* average time per multiplication (seconds) */

  /* FLOPs for one matrix multiply (approx): 2 * N^3 */
  double flops = 2.0 * (double)N * (double)N * (double)N;
  double gflops = (flops / elapsed) / 1e9;

  /* Print a succinct output */
  printf("N=%d reps=%d warmup=%d time_s=%.9f gflops=%.6f\n", N, reps, warmup,
         elapsed, gflops);

  free(A);
  free(B);
  free(C);
  return 0;
}
