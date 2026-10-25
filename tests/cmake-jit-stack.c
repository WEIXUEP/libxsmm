#include <libxsmm.h>
#include <math.h>
#include <stdio.h>

int main(void)
{
  /* Keep inputs off the stack so they do not precommit the JIT scratch pages. */
  static double a[128 * 256], b[256], c[128];
  const libxsmm_gemm_shape shape = libxsmm_create_gemm_shape(
    128, 1, 256, 256, 256, 128, LIBXSMM_DATATYPE_F64, LIBXSMM_DATATYPE_F64,
    LIBXSMM_DATATYPE_F64, LIBXSMM_DATATYPE_F64);
  libxsmm_gemm_param param = { 0 };
  libxsmm_gemmfunction kernel;
  int i, p;
  libxsmm_init();
  for (i = 0; i < 128 * 256; ++i) a[i] = ((i * 3) % 17 - 8) * 0.125;
  for (i = 0; i < 256; ++i) b[i] = ((i * 5) % 13 - 6) * 0.0625;
  kernel = libxsmm_dispatch_gemm(shape, LIBXSMM_GEMM_FLAG_TRANS_A | LIBXSMM_GEMM_FLAG_BETA_0,
    LIBXSMM_GEMM_PREFETCH_NONE);
  if (!kernel) return EXIT_FAILURE;
  param.a.primary = a;
  param.b.primary = b;
  param.c.primary = c;
  kernel(&param);
  for (i = 0; i < 128; ++i) {
    double expected = 0.0;
    for (p = 0; p < 256; ++p) expected += a[p + i * 256] * b[p];
    if (fabs(c[i] - expected) > 1.0e-12) {
      fprintf(stderr, "Large transposed FP64 JIT mismatch at %d\n", i);
      return EXIT_FAILURE;
    }
  }
  libxsmm_finalize();
  return EXIT_SUCCESS;
}
