# AVX2 vs Scalar Dot Product: Understanding the Numbers

## The four measurements

| Result | Compiler flags | Timing loop | Workload size |
|---|---|---|---|
| **67×** | `-O2 -mavx2 -mfma` | `s = dot(...)` | 64 MB arrays |
| **20×** | `-O3 -march=native -ffast-math` | `s = dot(...)` | 64 MB arrays |
| **1.02×** | `-O3 -march=native -ffast-math` | `s += dot(...)` + barrier, `noinline` | 64 MB arrays |
| **3.88×** | same as above | same as above | 16 KB arrays (L1) |

Each step fixed one specific lie the previous measurement was telling.

---

## The 67× run: scalar was crippled, AVX2 was elided

Two compounding problems.

**Scalar side: no auto-vectorization.** With `-O2` and no `-ffast-math`, the scalar loop was forced to compute one float at a time, strictly left-to-right. The compiler couldn't legally turn `sum += a[i]*b[i]` into a vectorized tree-sum, because FP addition isn't associative and reordering changes the result. So `dot_scalar` produced one `vmulss`/`vaddss` per element — scalar single-precision instructions that use only 1 lane of a vector register. The hardware capable of 8-wide SIMD was running at 1-wide.

**AVX2 side: the compiler skipped iterations.** The timing loop was `for (it = 0; it < ITERS; it++) s_avx2 = dot_avx2(a, b, N);` — note `=`, not `+=`. Only the last iteration's result is used. Combined with the function being pure (same inputs every time → same output), the compiler could:

- Recognize the previous 19 assignments as dead stores and eliminate them.
- Hoist the call out of the loop, computing `dot_avx2(a, b, N)` once and reusing the result.

So real memory traffic was ~one full pass through 128 MB, not 20 passes. We then divided that real time by 20 in our GB/s formula. That made the per-iteration time look ~20× smaller than it was, and the GB/s look ~20× larger — producing the impossible 552 GB/s number.

Multiply those two effects together — scalar artificially slow (no SIMD), AVX2 artificially fast (most iterations skipped) — and you get the bogus 67×. Almost none of that ratio reflects real SIMD parallelism.

---

## The 20× run: scalar fixed, AVX2 still elided

We added `-ffast-math` and `-O3`. That gives the compiler permission to reorder FP sums, and `-O3` turns on aggressive auto-vectorization. Now the scalar loop *also* gets compiled to 256-bit AVX2+FMA instructions — we confirmed this in the assembly (`ymm` registers, `vfmadd...ps` instructions).

So the scalar version dropped from ~15 ms to 4.6 ms — a ~3× improvement just from the compiler being allowed to vectorize. That alone accounts for most of the gap closure (67× → 20×).

But the AVX2 side was still showing 0.23 ms and "552 GB/s," because we hadn't touched the timing loop yet. The compiler was still hoisting the pure function call out of the loop. So we were comparing:

- A scalar loop running all 20 iterations honestly, ~27 GB/s DRAM-bound.
- An AVX2 loop where ~19 iterations were being skipped, with measured time divided by 20 anyway.

The 20× was the gap between an honest measurement of one and a dishonest measurement of the other. The scalar number was real; the AVX2 number was an artifact of the bogus division.

---

## The 1.02× run: both sides honest, memory is the bottleneck

We made three changes:

- Changed `s_avx2 = dot_avx2(...)` to `s_avx2 += dot_avx2(...)` — every iteration's result now contributes to the final printed sum, so none can be dead-eliminated.
- Added `asm volatile("" ::: "memory")` — a compiler barrier telling the compiler that memory could have changed, so it can't assume the function returns the same value across calls. This blocks loop-invariant code motion of the pure function call.
- Added `__attribute__((noinline))` to both functions, so they get called identically through real call instructions (the earlier asymmetry was that `dot_scalar` got inlined into `main` while `dot_avx2` stayed as a separate function).

Now both sides are running honestly — all 20 iterations of each loop actually execute. The result:

```
scalar : 27.22 GB/s   7.31 GFLOPS
avx2   : 27.65 GB/s   7.42 GFLOPS
```

**Both versions are bandwidth-bound on DRAM.** Your working set is 128 MB, far larger than your 12 MB L3 cache, so every iteration walks through the arrays as a stream — pulling cache lines from DRAM, processing them, evicting them by the time the next iteration starts. Nothing meaningfully persists in cache between iterations.

The dot product reads 8 bytes per element processed (4 bytes each from `a` and `b`) and does 2 FLOPs (multiply + add). That's an **arithmetic intensity of 0.25 FLOPs/byte** — very low.

Your DRAM delivers ~27 GB/s on this workload. At 0.25 FLOPs per byte loaded, that caps you at 27 × 0.25 ≈ 7 GFLOPS, regardless of how fast the FMA units are. The FMA units can do tens or hundreds of GFLOPS, but they're sitting idle most of the time, waiting for the next cache line to arrive from memory.

This is **the answer to a different question than we thought we were asking.** We thought we were measuring "how much does AVX2 speed up arithmetic," but on this workload the arithmetic isn't the bottleneck — the memory pipe is. AVX2 makes arithmetic faster, but if arithmetic isn't the bottleneck, faster arithmetic doesn't help. Both versions hit the same DRAM wall, both wait the same amount, both get the same time.

This is the realistic regime for most large-array numerical code.

---

## The 3.88× run: data in cache, compute is the bottleneck

We shrunk the arrays from 64 MB to 16 KB each. Total working set 32 KB, which fits exactly in your **32 KB per-core L1d cache**. Same code, same flags, same timing methodology — only the working-set size changed.

```
scalar : 65.80 GB/s    17.66 GFLOPS
avx2   : 255.22 GB/s   68.51 GFLOPS
speedup: 3.88×
```

Now the data lives in L1, which delivers hundreds of GB/s essentially for free. After the first warmup iteration loads everything into L1, all 100000 subsequent iterations hit cache with no DRAM traffic at all. The DRAM bottleneck is gone. The CPU is finally limited by how fast it can actually execute FMAs.

The AVX2 version hits 68 GFLOPS, the scalar 18. Both are real compute throughput, no memory effects. The 3.88× is the **honest SIMD speedup** for this workload.

Why 3.88× and not the theoretical 8×?

- The scalar version is also vectorized by the compiler (we saw `ymm` and packed FMA in the assembly), just not as aggressively unrolled as your hand-written 4-accumulator version. So the comparison isn't "1-wide vs 8-wide" — it's "less-well-tuned vectorization vs hand-tuned vectorization." A factor of ~4 between those is realistic.
- N=4096 is small. With 32 floats per main-loop iteration, that's only 128 iterations. Loop overhead, function-call cost, and the horizontal-sum reduction are a larger fraction of total runtime than they'd be on a longer loop.
- The compiler's vectorized scalar code probably uses fewer accumulators (1 or 2), so it doesn't fully hide FMA latency. Your hand-written 4-accumulator version does.

If you extrapolated to a truly compute-saturated workload — high arithmetic intensity, large inner loops, no memory-hierarchy effects — you'd see closer to 6–8× on this hardware.

---

## The unified picture

Each number was real in the sense that the program produced it. But they were measuring different things:

- **67×**: "How much faster is hand-tuned AVX2 than scalar the compiler isn't allowed to vectorize, when the AVX2 loop has 19 of 20 iterations skipped by the optimizer?" — meaningless ratio, two compounding artifacts.
- **20×**: "How much faster is hand-tuned AVX2 with most iterations skipped than honest auto-vectorized scalar?" — still meaningless, one artifact remaining.
- **1.02×**: "On memory-bound workloads, how much does SIMD help?" — answer: not at all, because the bottleneck isn't compute.
- **3.88×**: "On compute-bound workloads, how much does hand-tuned AVX2 beat compiler auto-vectorization?" — answer: roughly 4×, with a theoretical ceiling around 8×.

The last two are the only honest measurements. They answer different questions, and which one matters depends on what your real code looks like. For big arrays streamed from memory, you live in the 1× world and SIMD won't save you — you need to attack memory layout, prefetching, or just accept the ceiling. For small hot kernels operating on cache-resident data — inner loops of matmul, image filters on tiles, hash functions on small buffers — you live in the 4× world and SIMD is one of the biggest single wins available.

The methodological lesson is bigger than the numbers: **benchmarks routinely measure what the optimizer let happen rather than what you intended to measure.** Suspect any speedup that violates physics (552 GB/s from DRAM that does 27), any improvement that doesn't change with the relevant flag, any number that stays the same when you change something it should depend on. The number isn't wrong because the program is buggy — it's wrong because the model of what's being measured is wrong. The fix is always the same: poke at it, change one variable at a time, look at the assembly, and don't believe a result until you can predict it.
