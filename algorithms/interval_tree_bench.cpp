// Run in opt mode for accurate timings, with:
// bazel run --compilation_mode=opt algorithms:interval_tree_bench

#include <benchmark/benchmark.h>

#include <iostream>

#include "absl/random/random.h"
#include "interval_region.h"
#include "interval_tree.h"

static void BM_IntervalTree_Intersects(benchmark::State &state) {
  absl::BitGen gen;
  const auto tree = initRandomIntervalTree(state.range(0));

  // ADD THIS DIAGNOSTIC:
  int expected_depth = std::ceil(std::log2(state.range(0)));
  int actual_depth = tree.depth();
  if (actual_depth > expected_depth * 1.5) {
    std::cerr << "WARNING: Tree depth " << actual_depth << " >> expected "
              << expected_depth << " for n=" << state.range(0) << std::endl;
  }

  for (auto _ : state) {
    float q = absl::Uniform(gen, 0, 1.0);
    benchmark::DoNotOptimize(tree.queryIntervalTree(q));
  }
}

// Register the function as a benchmark
BENCHMARK(BM_IntervalTree_Intersects)->RangeMultiplier(2)->Range(32, 1e6);

BENCHMARK_MAIN();