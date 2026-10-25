#include <libxsmm.h>
#include <math.h>
#include <stdio.h>

int main(void)
{
  const libxsmm_blasint m = 5, n = 4, k = 3;
  double a[15], b[12], c[20];
  libxsmm_gemm_param param = { 0 };
  libxsmm_gemmfunction kernel;
  int beta, i, j, p;
  const libxsmm_gemm_shape shape = libxsmm_create_gemm_shape(
    m, n, k, m, k, m, LIBXSMM_DATATYPE_F64, LIBXSMM_DATATYPE_F64,
    LIBXSMM_DATATYPE_F64, LIBXSMM_DATATYPE_F64);

  libxsmm_init();
  for (i = 0; i < m * k; ++i) a[i] = (i - 7) * 0.25;
  for (i = 0; i < k * n; ++i) b[i] = (i + 1) * 0.125;
  for (beta = 0; beta <= 1; ++beta) {
    kernel = libxsmm_dispatch_gemm(shape,
      beta ? LIBXSMM_GEMM_FLAG_NONE : LIBXSMM_GEMM_FLAG_BETA_0,
      LIBXSMM_GEMM_PREFETCH_NONE);
    if (!kernel) {
      fprintf(stderr, "FP64 JIT kernel generation failed\n");
      return EXIT_FAILURE;
    }
    for (i = 0; i < m * n; ++i) c[i] = 2.0;
    param.a.primary = a;
    param.b.primary = b;
    param.c.primary = c;
    kernel(&param);
    for (j = 0; j < n; ++j) {
      for (i = 0; i < m; ++i) {
        double expected = beta ? 2.0 : 0.0;
        for (p = 0; p < k; ++p) expected += a[i + p * m] * b[p + j * k];
        if (fabs(c[i + j * m] - expected) > 1.0e-12) {
          fprintf(stderr, "FP64 JIT result mismatch at (%d, %d), beta=%d\n", i, j, beta);
          return EXIT_FAILURE;
        }
      }
    }
  }
  /* Non-square transposed GEMV is used for arbitrary-rule finite-element bases. */
  {
    const libxsmm_gemm_shape trans_shape = libxsmm_create_gemm_shape(
      3, 1, 5, 5, 5, 3, LIBXSMM_DATATYPE_F64, LIBXSMM_DATATYPE_F64,
      LIBXSMM_DATATYPE_F64, LIBXSMM_DATATYPE_F64);
    for (beta = 0; beta <= 1; ++beta) {
      kernel = libxsmm_dispatch_gemm(trans_shape,
        LIBXSMM_GEMM_FLAG_TRANS_A | (beta ? 0 : LIBXSMM_GEMM_FLAG_BETA_0), LIBXSMM_GEMM_PREFETCH_NONE);
      if (!kernel) return EXIT_FAILURE;
      for (i = 0; i < 3; ++i) c[i] = 2.0;
      param.a.primary = a;
      param.b.primary = b;
      param.c.primary = c;
      kernel(&param);
      for (i = 0; i < 3; ++i) {
        double expected = beta ? 2.0 : 0.0;
        for (p = 0; p < 5; ++p) expected += a[p + i * 5] * b[p];
        if (fabs(c[i] - expected) > 1.0e-12) {
          fprintf(stderr, "Transposed FP64 JIT mismatch at %d, beta=%d: %.17g != %.17g\n", i, beta, c[i], expected);
          return EXIT_FAILURE;
        }
      }
    }
  }
  libxsmm_finalize();
  return EXIT_SUCCESS;
}
