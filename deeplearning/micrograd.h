#ifndef DEEPLEARNING_MICROGRAD_H_
#define DEEPLEARNING_MICROGRAD_H_

// Following along with
// "The spelled-out intro to neural networks and backpropagation: building
// micrograd" -- https://www.youtube.com/watch?v=VMj-3S1tku0

#include <cmath>
#include <string>
#include <type_traits>
#include <unordered_set>

enum class ExprOp { Add, Mult, Tanh };

template <typename T>
class Value {
 public:
  // Simplest constructor is used to initialise "leaf" nodes (which are not the
  // result of an operation).
  Value(T val) : data(val) {}

  Value(T val, std::unordered_set<std::string> prev, ExprOp op)
      : data(val), children(prev), op(op) {}

  Value operator+(const Value<T>& other) const {
    return Value(data + other.data, {label, other.label}, ExprOp::Add);
  }

  Value operator*(const Value<T>& other) const {
    return Value(data * other.data, {label, other.label}, ExprOp::Mult);
  }

  Value tanh() const { return Value(std::tanh(data), {label}, ExprOp::Tanh); }

  bool operator==(const Value<T>& other) const {
    return data == other.data && label == other.label;
  }
  bool operator==(const Value<T>* other) const {
    return data == other->data && label == other->label;
  }

  T data;
  std::unordered_set<std::string> children;
  std::string label;
  ExprOp op;
  double grad = 0.;
};

template <typename T>
class ExprTree {
 public:
  const Value<T>& operator()(const std::string& label) const {
    // Throws if label doesn't exist. Just to keep the API compact.
    return nodes.at(label);
  }

  void reg(Value<T> expr, const std::string& label) {
    expr.label = label;
    nodes.emplace(label, std::move(expr));
  }

  void runBackpropRecursive(Value<T>& curr_node, const Value<T>& parent) {
    curr_node.grad += parent.grad;
    if (parent.op == ExprOp::Mult) {
      for (const auto& sib_label : parent.children) {
        auto& sib = nodes.at(sib_label);
        if (sib.label != curr_node.label) {
          curr_node.grad *= sib.data;
        }
      }
    } else if (parent.op == ExprOp::Add) {
      // Nothing. The partial deriv is 1.
    } else if (parent.op == ExprOp::Tanh) {
      curr_node.grad *= 1 - std::pow(parent.data, 2);
    }
    for (const auto& child_label : curr_node.children) {
      auto& child_node = nodes.at(child_label);
      this->runBackpropRecursive(child_node, curr_node);
    }
  }

  void runBackprop(const std::string& root_label) {
    auto& root = nodes.at(root_label);
    root.grad = 1.0;
    for (const auto& child_label : root.children) {
      auto& child_node = nodes.at(child_label);
      runBackpropRecursive(child_node, root);
    }
  }

  std::unordered_map<std::string, Value<T>> nodes;
};

#endif  // DEEPLEARNING_MICROGRAD_H_
