// avx2_bench.cpp
// Compare scalar vs AVX2 dot product of two float arrays.
//
// Build (recommended):
//   g++ -O3 -march=native -ffast-math -std=c++17 main.cpp -o avx2_bench
//
// Two modes — toggle via the #define below:
//   MODE_MEMORY  : N = 16M floats, ITERS = 20. Tests memory-bound behavior.
//   MODE_L1      : N = 4K  floats, ITERS = 100000. Tests pure compute throughput.

#include <immintrin.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cmath>
#include <random>

// ---- pick one ----
//#define MODE_MEMORY
#define MODE_L1

#ifdef MODE_MEMORY
  #define N     (1 << 24)   // 16M floats = 64 MB per array
  #define ITERS 20
#else
  #define N     (1 << 12)   // 4K floats = 16 KB per array, fits in L1 together
  #define ITERS 100000
#endif

// --- Scalar baseline -------------------------------------------------------
// noinline so it gets called as a real function — same optimization boundary
// as dot_avx2, so the comparison is apples-to-apples.
static __attribute__((noinline))
float dot_scalar(const float *a, const float *b, size_t n) {
    float sum = 0.0f;
    for (size_t i = 0; i < n; i++) {
        sum += a[i] * b[i];
    }
    return sum;
}

// --- AVX2 + FMA version ----------------------------------------------------
// 4 accumulators to hide FMA latency. Processes 32 floats per loop iteration.
static __attribute__((noinline))
float dot_avx2(const float *a, const float *b, size_t n) {
    __m256 acc0 = _mm256_setzero_ps();
    __m256 acc1 = _mm256_setzero_ps();
    __m256 acc2 = _mm256_setzero_ps();
    __m256 acc3 = _mm256_setzero_ps();

    size_t i = 0;
    size_t limit = n - (n % 32);
    for (; i < limit; i += 32) {
        __m256 a0 = _mm256_loadu_ps(a + i + 0);
        __m256 a1 = _mm256_loadu_ps(a + i + 8);
        __m256 a2 = _mm256_loadu_ps(a + i + 16);
        __m256 a3 = _mm256_loadu_ps(a + i + 24);
        __m256 b0 = _mm256_loadu_ps(b + i + 0);
        __m256 b1 = _mm256_loadu_ps(b + i + 8);
        __m256 b2 = _mm256_loadu_ps(b + i + 16);
        __m256 b3 = _mm256_loadu_ps(b + i + 24);
        acc0 = _mm256_fmadd_ps(a0, b0, acc0);
        acc1 = _mm256_fmadd_ps(a1, b1, acc1);
        acc2 = _mm256_fmadd_ps(a2, b2, acc2);
        acc3 = _mm256_fmadd_ps(a3, b3, acc3);
    }

    __m256 acc = _mm256_add_ps(_mm256_add_ps(acc0, acc1),
                               _mm256_add_ps(acc2, acc3));
    float tmp[8];
    _mm256_storeu_ps(tmp, acc);
    float sum = tmp[0] + tmp[1] + tmp[2] + tmp[3] +
                tmp[4] + tmp[5] + tmp[6] + tmp[7];

    for (; i < n; i++) sum += a[i] * b[i];
    return sum;
}

// --- Timing helper ---------------------------------------------------------
static double now_sec() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

int main() {
    auto *a = static_cast<float *>(aligned_alloc(32, N * sizeof(float)));
    auto *b = static_cast<float *>(aligned_alloc(32, N * sizeof(float)));
    if (!a || !b) {
        std::fprintf(stderr, "alloc failed\n");
        return 1;
    }

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    for (size_t i = 0; i < N; i++) {
        a[i] = dist(rng);
        b[i] = dist(rng);
    }

    // Warm up caches & touch pages.
    volatile float warm = dot_scalar(a, b, N) + dot_avx2(a, b, N);
    (void)warm;

    // --- Scalar ---
    // Accumulate each call's result and add a compiler barrier so the compiler
    // can't hoist or elide iterations.
    double t0 = now_sec();
    float s_scalar = 0;
    for (int it = 0; it < ITERS; it++) {
        s_scalar += dot_scalar(a, b, N);
        asm volatile("" ::: "memory");
    }
    double t_scalar = (now_sec() - t0) / ITERS;

    // --- AVX2 ---
    t0 = now_sec();
    float s_avx2 = 0;
    for (int it = 0; it < ITERS; it++) {
        s_avx2 += dot_avx2(a, b, N);
        asm volatile("" ::: "memory");
    }
    double t_avx2 = (now_sec() - t0) / ITERS;

    double gb = (2.0 * N * sizeof(float)) / (1024.0 * 1024.0 * 1024.0);
    double gflops_scalar = (2.0 * N) / (t_scalar * 1e9);
    double gflops_avx2   = (2.0 * N) / (t_avx2   * 1e9);

#ifdef MODE_MEMORY
    std::printf("MODE: MEMORY-BOUND (working set ~%.0f MB, larger than L3)\n",
                2.0 * N * sizeof(float) / (1024 * 1024));
#else
    std::printf("MODE: L1-RESIDENT (working set ~%.0f KB, fits in L1)\n",
                2.0 * N * sizeof(float) / 1024.0);
#endif
    std::printf("N = %d floats, iterations averaged: %d\n\n", N, ITERS);
    std::printf("scalar : %8.4f ms   %7.2f GB/s   %6.2f GFLOPS   result/iter = %.4f\n",
                t_scalar * 1000, gb / t_scalar, gflops_scalar, s_scalar / ITERS);
    std::printf("avx2   : %8.4f ms   %7.2f GB/s   %6.2f GFLOPS   result/iter = %.4f\n",
                t_avx2   * 1000, gb / t_avx2,   gflops_avx2,   s_avx2   / ITERS);
    std::printf("speedup: %.2fx\n", t_scalar / t_avx2);
    std::printf("max abs diff per iter: %.6f\n",
                std::abs(s_scalar / ITERS - s_avx2 / ITERS));

    std::free(a);
    std::free(b);
    return 0;
}
