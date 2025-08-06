#ifndef ALGORITHMS_INTERVAL_TREE_H_
#define ALGORITHMS_INTERVAL_TREE_H_

#include <memory>
#include <set>
#include <unordered_set>
#include <vector>

struct Interval {
  Interval(float a, float b) {
    if (a < b) {
      x_min = a;
      x_max = b;
    } else {
      x_min = b;
      x_max = a;
    }
  }

  // Comparison operator for std::set
  bool operator<(const Interval& other) const {
    if (x_min != other.x_min) return x_min < other.x_min;
    return x_max < other.x_max;
  }

  bool operator==(const Interval& other) const {
    return x_min == other.x_min && x_max == other.x_max;
  }

  bool contains(float query) const { return query >= x_min && query <= x_max; }
  float x_min, x_max;
  float y;
};

inline float getMedianEndpoint(const std::vector<Interval>& intervals) {
  // Populate the unique set of endpoints:
  std::unordered_set<float> interval_endpoints;

  for (const auto& interval : intervals) {
    interval_endpoints.insert(interval.x_min);
    interval_endpoints.insert(interval.x_max);
  }

  // Use the unique set to compute the median:
  std::vector<float> ordered_endpoints(interval_endpoints.begin(),
                                       interval_endpoints.end());
  auto m = ordered_endpoints.begin() + ordered_endpoints.size() / 2;
  std::nth_element(ordered_endpoints.begin(), m, ordered_endpoints.end());
  return ordered_endpoints[ordered_endpoints.size() / 2];
}

class IntervalTree {
 public:
  IntervalTree(std::vector<Interval> intervals) {
    x_mid = getMedianEndpoint(intervals);

    // Partition the input intervals into three:
    std::vector<Interval> left_intervals;
    std::vector<Interval> mid_intervals;
    std::vector<Interval> right_intervals;

    for (const auto& interval : intervals) {
      if (interval.x_max < x_mid) {
        left_intervals.push_back(interval);
      } else if (x_mid < interval.x_min) {
        right_intervals.push_back(interval);
      } else {
        mid_intervals.push_back(interval);
      }
    }

    // Recurse to populate child nodes:
    if (!left_intervals.empty()) {
      left = std::make_unique<IntervalTree>(std::move(left_intervals));
    }
    if (!right_intervals.empty()) {
      right = std::make_unique<IntervalTree>(std::move(right_intervals));
    }

    mid_left_sort = mid_intervals;
    mid_right_sort = mid_intervals;

    std::sort(
        mid_left_sort.begin(),
        mid_left_sort.end(),
        [](const Interval& a, const Interval& b) { return a.x_min < b.x_min; });

    std::sort(
        mid_right_sort.begin(),
        mid_right_sort.end(),
        [](const Interval& a, const Interval& b) { return a.x_max > b.x_max; });
  }

  int depth() const {
    int left_depth = left == nullptr ? 0 : left->depth();
    int right_depth = right == nullptr ? 0 : right->depth();
    return 1 + std::max(left_depth, right_depth);
  }

  static void populateMatchesFromSortedIntervals(
      const std::vector<Interval>& intervals,
      std::set<Interval>& matches,
      float query) {
    for (const auto& interval : intervals) {
      if (interval.contains(query)) {
        matches.insert(interval);
      } else {
        // Stop as soon as an interval does not contain query.
        return;
      }
    }
  }

  std::set<Interval> queryIntervalTree(float query) const {
    std::set<Interval> matches;

    if (query < x_mid) {
      populateMatchesFromSortedIntervals(mid_left_sort, matches, query);
      if (left != nullptr) {
        auto left_matches = left->queryIntervalTree(query);
        matches.insert(left_matches.begin(), left_matches.end());
      }
    } else {
      populateMatchesFromSortedIntervals(mid_right_sort, matches, query);
      if (right != nullptr) {
        auto right_matches = right->queryIntervalTree(query);
        matches.insert(right_matches.begin(), right_matches.end());
      }
    }

    return matches;
  }

  float x_mid;  // Median of the set of interval endpoints.

  std::vector<Interval> mid_left_sort;
  std::vector<Interval> mid_right_sort;

  std::unique_ptr<IntervalTree> left;
  std::unique_ptr<IntervalTree> right;
};

#endif  // ALGORITHMS_INTERVAL_TREE_H_