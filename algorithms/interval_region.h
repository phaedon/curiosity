#include "absl/random/random.h"
#include "interval_tree.h"

IntervalTree initRandomIntervalTree(size_t num_intervals) {
  absl::BitGen gen;
  std::vector<Interval> intervals;

  for (size_t i = 0; i < num_intervals; ++i) {
    float start = absl::Uniform(gen, 0.0, 0.9);  // Leave room for width
    // 80% small intervals, 20% large
    float width = (absl::Uniform(gen, 0.0, 1.0) < 0.95)
                      ? absl::Uniform(gen, 0.01, 0.03)  // small
                      : absl::Uniform(gen, 0.05, 0.1);  // large
    auto interval = Interval(start, start + width);
    interval.y = absl::Uniform(gen, 0, 1.0);
    intervals.push_back(interval);
  }

  return IntervalTree(std::move(intervals));
}