// avx2_bench.cpp
// g++ -O3 -march=native -ffast-math -std=c++17 avx2_bench.cpp \
//     -lbenchmark -lpthread -o avx2_bench
//
// Override L1d size (bytes) for your CPU with -DL1D_BYTES=N, e.g.:
//   g++ ... -DL1D_BYTES=49152   # 48 KB L1d (e.g. Zen 4)
//   getconf LEVEL1_DCACHE_SIZE  # prints bytes on Linux
//
// Run:
//   ./avx2_bench
//   ./avx2_bench --benchmark_repetitions=10 --benchmark_report_aggregation=median
//   ./avx2_bench --benchmark_filter=L1

#include <benchmark/benchmark.h>
#include <immintrin.h>
#include <random>
#include <cstdlib>

// Bytes of L1d cache per core. Default = 32 KB (common Intel/AMD value).
// Override at compile time: -DL1D_BYTES=49152 for 48 KB, etc.
#ifndef L1D_BYTES
#define L1D_BYTES 32768
#endif

static constexpr size_t N_MEMORY = 1 << 24;  // 16M floats = 64 MB per array
// Each array occupies half of L1d so both arrays together exactly fill the cache.
static constexpr size_t N_L1 = (L1D_BYTES / 2) / sizeof(float);

// ---------------------------------------------------------------------------
// Kernel implementations
// ---------------------------------------------------------------------------

static __attribute__((noinline))
float dot_scalar(const float* __restrict__ a, const float* __restrict__ b, size_t n) {
    float sum = 0.0f;
    for (size_t i = 0; i < n; i++)
        sum += a[i] * b[i];
    return sum;
}

static __attribute__((noinline))
float dot_avx2(const float* __restrict__ a, const float* __restrict__ b, size_t n) {
    __m256 acc0 = _mm256_setzero_ps();
    __m256 acc1 = _mm256_setzero_ps();
    __m256 acc2 = _mm256_setzero_ps();
    __m256 acc3 = _mm256_setzero_ps();

    size_t i = 0;
    const size_t limit = n - (n % 32);
    for (; i < limit; i += 32) {
        acc0 = _mm256_fmadd_ps(_mm256_loadu_ps(a+i+ 0), _mm256_loadu_ps(b+i+ 0), acc0);
        acc1 = _mm256_fmadd_ps(_mm256_loadu_ps(a+i+ 8), _mm256_loadu_ps(b+i+ 8), acc1);
        acc2 = _mm256_fmadd_ps(_mm256_loadu_ps(a+i+16), _mm256_loadu_ps(b+i+16), acc2);
        acc3 = _mm256_fmadd_ps(_mm256_loadu_ps(a+i+24), _mm256_loadu_ps(b+i+24), acc3);
    }

    __m256 acc = _mm256_add_ps(_mm256_add_ps(acc0, acc1),
                               _mm256_add_ps(acc2, acc3));
    float tmp[8];
    _mm256_storeu_ps(tmp, acc);
    float sum = tmp[0]+tmp[1]+tmp[2]+tmp[3]+tmp[4]+tmp[5]+tmp[6]+tmp[7];

    for (; i < n; i++) sum += a[i] * b[i];
    return sum;
}

// AVX (256-bit, no FMA): uses vmulps + vaddps — one fewer execution port
// than FMA but available on any AVX-capable CPU, showing AVX vs AVX2 gap.
static __attribute__((noinline))
float dot_avx(const float* __restrict__ a, const float* __restrict__ b, size_t n) {
    __m256 acc0 = _mm256_setzero_ps();
    __m256 acc1 = _mm256_setzero_ps();
    __m256 acc2 = _mm256_setzero_ps();
    __m256 acc3 = _mm256_setzero_ps();

    size_t i = 0;
    const size_t limit = n - (n % 32);
    for (; i < limit; i += 32) {
        acc0 = _mm256_add_ps(acc0, _mm256_mul_ps(_mm256_loadu_ps(a+i+ 0), _mm256_loadu_ps(b+i+ 0)));
        acc1 = _mm256_add_ps(acc1, _mm256_mul_ps(_mm256_loadu_ps(a+i+ 8), _mm256_loadu_ps(b+i+ 8)));
        acc2 = _mm256_add_ps(acc2, _mm256_mul_ps(_mm256_loadu_ps(a+i+16), _mm256_loadu_ps(b+i+16)));
        acc3 = _mm256_add_ps(acc3, _mm256_mul_ps(_mm256_loadu_ps(a+i+24), _mm256_loadu_ps(b+i+24)));
    }

    __m256 acc = _mm256_add_ps(_mm256_add_ps(acc0, acc1),
                               _mm256_add_ps(acc2, acc3));
    float tmp[8];
    _mm256_storeu_ps(tmp, acc);
    float sum = tmp[0]+tmp[1]+tmp[2]+tmp[3]+tmp[4]+tmp[5]+tmp[6]+tmp[7];

    for (; i < n; i++) sum += a[i] * b[i];
    return sum;
}

// ---------------------------------------------------------------------------
// Fixture: allocates and initialises arrays once per benchmark family
// ---------------------------------------------------------------------------

struct DotFixture : benchmark::Fixture {
    float* a = nullptr;
    float* b = nullptr;
    size_t N = 0;

    void Init(size_t n) {
        N = n;
        a = static_cast<float*>(aligned_alloc(32, N * sizeof(float)));
        b = static_cast<float*>(aligned_alloc(32, N * sizeof(float)));

        std::mt19937 rng(42);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        for (size_t i = 0; i < N; i++) { a[i] = dist(rng); b[i] = dist(rng); }

        // One warmup pass so page faults don't pollute the first iteration.
        benchmark::DoNotOptimize(dot_scalar(a, b, N));
        benchmark::DoNotOptimize(dot_avx(a, b, N));
        benchmark::DoNotOptimize(dot_avx2(a, b, N));
    }

    void TearDown(const benchmark::State&) override {
        std::free(a); std::free(b);
        a = b = nullptr;
    }
};

// ---------------------------------------------------------------------------
// MEMORY-BOUND benchmarks  (64 MB working set, exceeds L3)
// ---------------------------------------------------------------------------

BENCHMARK_F(DotFixture, Memory_Scalar)(benchmark::State& state) {
    Init(N_MEMORY);
    float sink = 0.0f;
    for (auto _ : state) {
        sink += dot_scalar(a, b, N);
        benchmark::DoNotOptimize(sink);
        benchmark::ClobberMemory();
    }
    state.SetBytesProcessed(state.iterations() * 2LL * N * sizeof(float));
    state.SetItemsProcessed(state.iterations() * 2LL * N);  // items = FLOPs
    state.counters["GFLOPS"] = benchmark::Counter(
        state.iterations() * 2.0 * N,
        static_cast<benchmark::Counter::Flags>(benchmark::Counter::kIsRate | benchmark::Counter::kIs1000),
        benchmark::Counter::kIs1000);
}

BENCHMARK_F(DotFixture, Memory_AVX)(benchmark::State& state) {
    Init(N_MEMORY);
    float sink = 0.0f;
    for (auto _ : state) {
        sink += dot_avx(a, b, N);
        benchmark::DoNotOptimize(sink);
        benchmark::ClobberMemory();
    }
    state.SetBytesProcessed(state.iterations() * 2LL * N * sizeof(float));
    state.SetItemsProcessed(state.iterations() * 2LL * N);
    state.counters["GFLOPS"] = benchmark::Counter(
        state.iterations() * 2.0 * N,
        static_cast<benchmark::Counter::Flags>(benchmark::Counter::kIsRate | benchmark::Counter::kIs1000),
        benchmark::Counter::kIs1000);
}

BENCHMARK_F(DotFixture, Memory_AVX2)(benchmark::State& state) {
    Init(N_MEMORY);
    float sink = 0.0f;
    for (auto _ : state) {
        sink += dot_avx2(a, b, N);
        benchmark::DoNotOptimize(sink);
        benchmark::ClobberMemory();
    }
    state.SetBytesProcessed(state.iterations() * 2LL * N * sizeof(float));
    state.SetItemsProcessed(state.iterations() * 2LL * N);
    state.counters["GFLOPS"] = benchmark::Counter(
        state.iterations() * 2.0 * N,
        static_cast<benchmark::Counter::Flags>(benchmark::Counter::kIsRate | benchmark::Counter::kIs1000),
        benchmark::Counter::kIs1000);
}

// ---------------------------------------------------------------------------
// L1-RESIDENT benchmarks  (L1D_BYTES working set, fills L1d exactly)
// ---------------------------------------------------------------------------

BENCHMARK_F(DotFixture, L1_Scalar)(benchmark::State& state) {
    Init(N_L1);
    float sink = 0.0f;
    for (auto _ : state) {
        sink += dot_scalar(a, b, N);
        benchmark::DoNotOptimize(sink);
        benchmark::ClobberMemory();
    }
    state.SetBytesProcessed(state.iterations() * 2LL * N * sizeof(float));
    state.SetItemsProcessed(state.iterations() * 2LL * N);
    state.counters["GFLOPS"] = benchmark::Counter(
        state.iterations() * 2.0 * N,
        static_cast<benchmark::Counter::Flags>(benchmark::Counter::kIsRate | benchmark::Counter::kIs1000),
        benchmark::Counter::kIs1000);
}

BENCHMARK_F(DotFixture, L1_AVX)(benchmark::State& state) {
    Init(N_L1);
    float sink = 0.0f;
    for (auto _ : state) {
        sink += dot_avx(a, b, N);
        benchmark::DoNotOptimize(sink);
        benchmark::ClobberMemory();
    }
    state.SetBytesProcessed(state.iterations() * 2LL * N * sizeof(float));
    state.SetItemsProcessed(state.iterations() * 2LL * N);
    state.counters["GFLOPS"] = benchmark::Counter(
        state.iterations() * 2.0 * N,
        static_cast<benchmark::Counter::Flags>(benchmark::Counter::kIsRate | benchmark::Counter::kIs1000),
        benchmark::Counter::kIs1000);
}

BENCHMARK_F(DotFixture, L1_AVX2)(benchmark::State& state) {
    Init(N_L1);
    float sink = 0.0f;
    for (auto _ : state) {
        sink += dot_avx2(a, b, N);
        benchmark::DoNotOptimize(sink);
        benchmark::ClobberMemory();
    }
    state.SetBytesProcessed(state.iterations() * 2LL * N * sizeof(float));
    state.SetItemsProcessed(state.iterations() * 2LL * N);
    state.counters["GFLOPS"] = benchmark::Counter(
        state.iterations() * 2.0 * N,
        static_cast<benchmark::Counter::Flags>(benchmark::Counter::kIsRate | benchmark::Counter::kIs1000),
        benchmark::Counter::kIs1000);
}

BENCHMARK_MAIN();
