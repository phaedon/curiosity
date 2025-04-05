#include "micrograd.h"

#include <gtest/gtest.h>

#include "gmock/gmock.h"

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

TEST(MicrogradTest, ValueObjectAPI) {
  EXPECT_EQ(Value(5), Value(2) + Value(3));
  EXPECT_EQ(Value(6), Value(2) * Value(3));

  auto d = Value(2) * Value(-3) + Value(10);
  EXPECT_EQ(Value(4), d);
  EXPECT_THAT(d.children, testing::UnorderedElementsAre(Value(-6), Value(10)));

  // Next steps:
  // https://www.boost.org/doc/libs/1_87_0/libs/graph/doc/write-graphviz.html at
  // boost/graph/graphviz.hpp
}