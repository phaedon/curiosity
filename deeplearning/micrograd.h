#ifndef DEEPLEARNING_MICROGRAD_H_
#define DEEPLEARNING_MICROGRAD_H_

// Following along with
// "The spelled-out intro to neural networks and backpropagation: building
// micrograd" -- https://www.youtube.com/watch?v=VMj-3S1tku0

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/graphviz.hpp>
#include <cmath>
#include <string>
#include <type_traits>
#include <unordered_set>

#include "absl/strings/str_cat.h"

// Here's a parabola.
inline double f(double x) { return 3 * std::pow(x, 2) - 4 * x + 5; }

// And here's an arbitrary example function with multiple variables.
inline double g(double a, double b, double c) { return a * b + c; }

template <typename T>
class Value;  // Forward declaration

template <typename T>
struct ValueHash {
  std::size_t operator()(const Value<T>& v) const {
    std::size_t h1 = std::hash<T>{}(v.data);
    std::size_t h2 = std::hash<std::string>{}(v.label);
    return h1 ^ (h2 << 1);  // Combine hashes
  }
};

template <typename T>
class Value {
 public:
  Value(T val, std::string lab = "") : data(val), label(lab) {}

  Value(T val,
        std::unordered_set<Value<T>, ValueHash<T>> prev,
        std::string lab = "",
        std::string op = "")
      : data(val), children(prev), op(op), label(lab) {}

  Value operator+(const Value<T>& other) const {
    std::unordered_set<Value<T>, ValueHash<T>> prev = {*this, other};

    return Value(
        data + other.data, prev, absl::StrCat(label, other.label), "+");
  }

  Value operator*(const Value<T>& other) const {
    std::unordered_set<Value<T>, ValueHash<T>> prev = {*this, other};

    return Value(
        data * other.data, prev, absl::StrCat(label, other.label), "*");
  }

  Value tanh() const {
    return Value(std::tanh(data), {*this}, absl::StrCat("tanh", label), "tanh");
  }

  bool operator==(const Value<T>& other) const {
    return data == other.data && label == other.label;
  }
  bool operator==(const Value<T>* other) const {
    return data == other->data && label == other->label;
  }

  T data;
  std::unordered_set<Value<T>, ValueHash<T>> children;
  std::string op;
  std::string label;
  double grad = 0.;
};

// Define the graph type with a vertex property for the label
struct NodeProperties {
  std::string label;
  std::string op;
};

using Graph = boost::adjacency_list<boost::vecS,       // OutEdgeList
                                    boost::vecS,       // VertexList
                                    boost::directedS,  // Directed
                                    NodeProperties     // VertexProperties
                                    >;
using Vertex = boost::graph_traits<Graph>::vertex_descriptor;

template <typename T>
void build_value_graph_recursive(
    const Value<T>& v,
    std::unordered_map<const Value<T>*, Vertex>& value_to_vertex,
    Graph& g,
    std::unordered_set<const Value<T>*>& visited) {
  if (visited.contains(&v)) {
    return;
  }
  visited.insert(&v);

  if (!value_to_vertex.contains(&v)) {
    Vertex current_v = boost::add_vertex(g);
    g[current_v].label = (v.label.empty() ? std::to_string(v.data) : v.label) +
                         "\nvalue: " + std::to_string(v.data) +
                         "\ngrad: " + std::to_string(v.grad);
    g[current_v].op = "";
    value_to_vertex[&v] = current_v;
  }
  Vertex current_v = value_to_vertex[&v];

  if (v.children.empty()) {
    return;
  }

  Vertex op_v = boost::add_vertex(g);
  g[op_v].label = v.op;
  g[op_v].op = v.op;

  // Edges from operands to the operator
  for (const Value<T>& child : v.children) {
    if (!value_to_vertex.contains(&child)) {
      Vertex child_v = boost::add_vertex(g);
      g[child_v].label =
          (child.label.empty() ? std::to_string(child.data) : child.label) +
          "\nvalue: " + std::to_string(child.data) +
          "\ngrad: " + std::to_string(child.grad);
      value_to_vertex[&child] = child_v;
      build_value_graph_recursive(child, value_to_vertex, g, visited);
    }
    boost::add_edge(
        value_to_vertex[&child], op_v, g);  // Operand points to operator
  }

  // Edge from the operator to the result
  boost::add_edge(op_v, current_v, g);

  // Recursively process children AFTER creating the operator and operand edges
  for (const Value<T>& child : v.children) {
    build_value_graph_recursive(child, value_to_vertex, g, visited);
  }
}

template <typename T>
Graph build_value_graph_with_ops(const Value<T>& root) {
  Graph g;
  std::unordered_map<const Value<T>*, Vertex> value_to_vertex;
  std::unordered_set<const Value<T>*> visited;
  build_value_graph_recursive(root, value_to_vertex, g, visited);
  return g;
}

inline void write_dot_file(const Graph& g, const std::string& filename) {
  std::ofstream dot_file(filename);
  if (dot_file.is_open()) {
    boost::write_graphviz(
        dot_file,
        g,
        boost::make_label_writer(boost::get(&NodeProperties::label, g)));
    dot_file.close();
    std::cout << "Graph written to " << filename << std::endl;
  } else {
    std::cerr << "Error opening file: " << filename << std::endl;
  }
}

template <typename T>
inline void runBackpropRecursive(Value<T>& curr_node, const Value<T>& parent) {
  curr_node.grad = parent.grad;
  if (parent.op == "*") {
    for (const auto& sib : parent.children) {
      if (sib.label != curr_node.label) {
        curr_node.grad *= sib.data;
      }
    }
  } else if (parent.op == "+") {
  } else if (parent.op == "tanh") {
    curr_node.grad *= 1 - std::pow(parent.data, 2);
  }
  for (const Value<T>& child : curr_node.children) {
    runBackpropRecursive<T>(const_cast<Value<T>&>(child), curr_node);
  }
}

template <typename T>
inline void runBackprop(Value<T>& root) {
  root.grad = 1.0;
  for (const Value<T>& child : root.children) {
    runBackpropRecursive<T>(const_cast<Value<T>&>(child), root);
  }
}

#endif  // DEEPLEARNING_MICROGRAD_H_
