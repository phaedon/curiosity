#include "interval_tree.h"

#include <gtest/gtest.h>

#include "gmock/gmock.h"

using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

TEST(IntervalTreeTest, Textbook) {
  // This test reproduces the tree displayed on pg 223 of
  // "Computational Geometry" by de Berg et al. (3rd ed.)
  IntervalTree example({Interval(1, 5),
                        Interval(2, 3),
                        Interval(4, 10),
                        Interval(6, 8),
                        Interval(7, 14),
                        Interval(9, 12),
                        Interval(11, 13)});

  EXPECT_EQ(2, example.depth());

  EXPECT_THAT(
      example.queryIntervalTree(7.5),
      UnorderedElementsAre(Interval(4, 10), Interval(6, 8), Interval(7, 14)));

  EXPECT_THAT(example.queryIntervalTree(5.5),
              UnorderedElementsAre(Interval(4, 10)));

  EXPECT_THAT(example.queryIntervalTree(0.5), IsEmpty());
  EXPECT_THAT(example.queryIntervalTree(15.5), IsEmpty());
}