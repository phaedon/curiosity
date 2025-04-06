
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/graphviz.hpp>

#include "micrograd.h"

// Define the graph type with a vertex property for the label
struct NodeProperties {
  std::string label;
  std::string op;
};

std::string to_string(ExprOp op) {
  switch (op) {
    case ExprOp::Add:
      return "+";
    case ExprOp::Mult:
      return "*";
    case ExprOp::Tanh:
      return "tanh";
    default:
      return "";
  }
}

using Graph = boost::adjacency_list<boost::vecS,       // OutEdgeList
                                    boost::vecS,       // VertexList
                                    boost::directedS,  // Directed
                                    NodeProperties     // VertexProperties
                                    >;
using Vertex = boost::graph_traits<Graph>::vertex_descriptor;
template <typename T>
void build_value_graph_recursive(
    const ExprTree<T>& tree,     // Pass ExprTree as a parameter
    const std::string& v_label,  // Use string labels
    std::unordered_map<const std::string*, Vertex>&
        label_to_vertex,  // Map string labels to vertices
    Graph& g,
    std::unordered_set<const std::string*>& visited) {
  if (visited.contains(&v_label)) {
    return;
  }
  visited.insert(&v_label);

  if (!label_to_vertex.contains(&v_label)) {
    Vertex current_v = boost::add_vertex(g);
    const Value<T>& v = tree(v_label);  // Access Value object from ExprTree
    g[current_v].label = (v.label.empty() ? std::to_string(v.data) : v.label) +
                         "\nvalue: " + std::to_string(v.data) +
                         "\ngrad: " + std::to_string(v.grad);
    g[current_v].op = "";
    label_to_vertex[&v_label] = current_v;
  }
  Vertex current_v = label_to_vertex[&v_label];

  const Value<T>& v = tree(v_label);  // Access Value object from ExprTree

  if (v.children.empty()) {
    return;
  }

  Vertex op_v = boost::add_vertex(g);
  g[op_v].label = to_string(v.op);  // Convert ExprOp to string
  g[op_v].op = to_string(v.op);
  boost::add_edge(op_v, current_v, g);

  // Edges from operands to the operator
  for (const std::string& child_label : v.children) {
    if (!label_to_vertex.contains(&child_label)) {
      Vertex child_v = boost::add_vertex(g);
      const Value<T>& child =
          tree(child_label);  // Access Value object from ExprTree
      g[child_v].label =
          (child.label.empty() ? std::to_string(child.data) : child.label) +
          "\nvalue: " + std::to_string(child.data) +
          "\ngrad: " + std::to_string(child.grad);
      g[child_v].op = "";
      label_to_vertex[&child_label] = child_v;
      build_value_graph_recursive(
          tree, child_label, label_to_vertex, g, visited);
    }
    boost::add_edge(
        label_to_vertex[&child_label], op_v, g);  // Operand points to operator
  }

  // Edge from the operator to the result
  // boost::add_edge(op_v, current_v, g);

  // Recursively process children AFTER creating the operator and operand edges
  for (const std::string& child_label : v.children) {
    build_value_graph_recursive(tree, child_label, label_to_vertex, g, visited);
  }
}

template <typename T>
Graph build_value_graph_with_ops(const ExprTree<T>& tree,
                                 const std::string& root_label) {
  Graph g;
  std::unordered_map<const std::string*, Vertex> label_to_vertex;
  std::unordered_set<const std::string*> visited;
  build_value_graph_recursive(tree, root_label, label_to_vertex, g, visited);
  return g;
}

inline void write_dot_file(const Graph& g, const std::string& filename) {
  std::ofstream dot_file(filename);
  if (dot_file.is_open()) {
    boost::write_graphviz(dot_file, g, [&](std::ostream& out, const Vertex& v) {
      out << "[label=\"" << g[v].label << "\"";
      if (!g[v].op.empty()) {
        out << " shape=ellipse";
      } else {
        out << " shape=box";
      }
      out << "]";
    });
    dot_file.close();
    std::cout << "Graph written to " << filename << std::endl;
  } else {
    std::cerr << "Error opening file: " << filename << std::endl;
  }
}

void devNewAPI() {
  ExprTree<double> tree;

  tree.reg(Value(-2.), "a");
  tree.reg(Value(3.), "b");
  tree.reg(tree("a") * tree("b"), "d");
  tree.reg(tree("a") + tree("b"), "e");
  tree.reg(tree("d") * tree("e"), "f");
  tree.runBackprop("f");

  Graph graph = build_value_graph_with_ops(tree, "f");
  write_dot_file(graph, "multiedge.dot");
}

int main() {
  // Manual backprop example #2: A neuron
  ExprTree<double> neuron;
  neuron.reg(Value(2.0), "x1");
  neuron.reg(Value(0.0), "x2");
  neuron.reg(Value(-3.0), "w1");
  neuron.reg(Value(1.0), "w2");

  neuron.reg(Value(6.8813735870195432), "bias");
  neuron.reg(neuron("x1") * neuron("w1"), "x1w1");
  neuron.reg(neuron("x2") * neuron("w2"), "x2w2");
  neuron.reg(neuron("x1w1") + neuron("x2w2"), "x1w1+x2w2");
  neuron.reg(neuron("x1w1+x2w2") + neuron("bias"), "n");
  neuron.reg(neuron("n").tanh(), "o");

  neuron.runBackprop("o");
  Graph graph2 = build_value_graph_with_ops(neuron, "o");
  write_dot_file(graph2, "neuron.dot");

  devNewAPI();

  return 0;
}