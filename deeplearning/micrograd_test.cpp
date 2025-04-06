#include "micrograd.h"

#include <gtest/gtest.h>

#include "gmock/gmock.h"

// Here's a parabola.
inline double f(double x) { return 3 * std::pow(x, 2) - 4 * x + 5; }

TEST(MicrogradTest, Parabola) {
  EXPECT_NEAR(5.0, f(0), 0.0001);

  // Numerical approximation of the derivative of the function f
  // at a specific point.
  double h = 0.00001;
  double x = 3.0;
  EXPECT_NEAR(14, (f(x + h) - f(x)) / h, 1e-4);

  // Point at which the slope is zero.
  x = 2. / 3;
  EXPECT_NEAR(0, (f(x + h) - f(x)) / h, 1e-4);
}

// And here's an arbitrary example function with multiple variables.
inline double g(double a, double b, double c) { return a * b + c; }

TEST(MicrogradTest, MultivarFn) {
  double a = 2.;
  double b = -3;
  double c = 10.;

  double h = 0.0001;
  double d1 = g(a, b, c);
  double d2 = g(a + h, b, c);
  EXPECT_NEAR(4., d1, 0.0001);
  // The partial derivative wrt a is expected to be negative.
  EXPECT_LT((d2 - d1) / h, 0);

  // Partial wrt b is expected to be positive.
  EXPECT_GT((g(a, b + h, c) - d1) / h, 0);
}

TEST(MicrogradTest, BackpropWithSharedNodes) {
  // This test reproduces the graph at
  // https://youtu.be/VMj-3S1tku0?t=5194
  ExprTree<double> tree;

  tree.reg(Value(-2.), "a");
  tree.reg(Value(3.), "b");
  tree.reg(tree("a") * tree("b"), "d");
  tree.reg(tree("a") + tree("b"), "e");
  tree.reg(tree("d") * tree("e"), "f");
  tree.runBackprop("f");

  EXPECT_EQ(5, tree.nodes.size());

  EXPECT_DOUBLE_EQ(-6, tree("f").data);
  EXPECT_DOUBLE_EQ(1.0, tree("f").grad);

  EXPECT_DOUBLE_EQ(1.0, tree("e").data);
  EXPECT_DOUBLE_EQ(-6.0, tree("e").grad);

  EXPECT_DOUBLE_EQ(-6.0, tree("d").data);
  EXPECT_DOUBLE_EQ(1.0, tree("d").grad);

  EXPECT_DOUBLE_EQ(-2.0, tree("a").data);
  EXPECT_DOUBLE_EQ(-3.0, tree("a").grad);

  EXPECT_DOUBLE_EQ(3.0, tree("b").data);
  EXPECT_DOUBLE_EQ(-8.0, tree("b").grad);
}