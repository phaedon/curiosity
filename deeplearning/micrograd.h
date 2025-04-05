// Following along with
// "The spelled-out intro to neural networks and backpropagation: building
// micrograd" -- https://www.youtube.com/watch?v=VMj-3S1tku0

#include <cmath>
#include <unordered_set>

// Here's a parabola.
double f(double x) { return 3 * std::pow(x, 2) - 4 * x + 5; }

double g(double a, double b, double c) { return a * b + c; }

class Value {
 public:
  Value(double val) : data(val) {}

  Value(double val,
        const std::unordered_set<const Value*>& prev,
        char op = 0,
        std::string lab = "")
      : data(val), children(prev), op(op), label(lab) {}

  Value operator+(const Value& other) const {
    return Value(data + other.data, {this, &other}, '+');
  }

  Value operator*(const Value& other) const {
    return Value(data * other.data, {this, &other}, '*');
  }

  bool operator==(const Value& other) const { return data == other.data; }
  bool operator==(const Value* other) const { return data == other->data; }

  double data;
  std::unordered_set<const Value*> children;
  char op;
  std::string label;
};