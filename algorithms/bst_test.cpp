
#include "bst.h"

#include <gtest/gtest.h>

#include "gmock/gmock.h"

using ::testing::UnorderedElementsAre;

TEST(BSTTest, RangeQuery_EmptyTree) {
  BST<float> tree;
  EXPECT_TRUE(tree.rangeQuery1D(-10, 10).empty());
}

TEST(BSTTest, RangeQuery_SingleNode) {
  BST<float> tree;
  tree.insert(5);
  EXPECT_TRUE(tree.rangeQuery1D(-10, 0).empty());
  EXPECT_TRUE(tree.rangeQuery1D(10, 20).empty());
  EXPECT_THAT(tree.rangeQuery1D(4, 6), UnorderedElementsAre(5));

  // Exact match.
  EXPECT_THAT(tree.rangeQuery1D(5, 5), UnorderedElementsAre(5));
}

TEST(BSTTest, RangeQuery_BalancedTree) {
  BST<float> tree;
  std::vector<float> values = {5, 3, 7, 1, 4, 6, 9};
  //  std::vector<float> values = {1, 3, 4, 5, 6, 7, 9};
  // std::vector<float> values = {9, 7, 6, 5, 4, 3, 1};
  for (float v : values) tree.insert(v);

  EXPECT_THAT(tree.rangeQuery1D(3, 7), UnorderedElementsAre(3, 4, 5, 6, 7));
  EXPECT_THAT(tree.rangeQuery1D(0, 2), UnorderedElementsAre(1));
  EXPECT_THAT(tree.rangeQuery1D(8, 10), UnorderedElementsAre(9));
  EXPECT_THAT(tree.rangeQuery1D(4.5, 5.5), UnorderedElementsAre(5));
}

TEST(BSTTest, RangeQuery_UnbalancedTrees) {
  BST<float> asctree, desctree;
  std::vector<float> ascending = {1, 3, 4, 5, 6, 7, 9};
  std::vector<float> descending = {9, 7, 6, 5, 4, 3, 1};
  for (float v : ascending) asctree.insert(v);
  for (float v : descending) desctree.insert(v);

  EXPECT_THAT(asctree.rangeQuery1D(3, 7), UnorderedElementsAre(3, 4, 5, 6, 7));
  EXPECT_THAT(asctree.rangeQuery1D(0, 2), UnorderedElementsAre(1));
  EXPECT_THAT(asctree.rangeQuery1D(8, 10), UnorderedElementsAre(9));
  EXPECT_THAT(asctree.rangeQuery1D(4.5, 5.5), UnorderedElementsAre(5));

  EXPECT_THAT(desctree.rangeQuery1D(3, 7), UnorderedElementsAre(3, 4, 5, 6, 7));
  EXPECT_THAT(desctree.rangeQuery1D(0, 2), UnorderedElementsAre(1));
  EXPECT_THAT(desctree.rangeQuery1D(8, 10), UnorderedElementsAre(9));
  EXPECT_THAT(desctree.rangeQuery1D(4.5, 5.5), UnorderedElementsAre(5));
}