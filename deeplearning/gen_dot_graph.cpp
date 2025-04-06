
#include "micrograd.h"

int main() {
  // This version generates a correct graph:
  auto a = Value<int>(2, "a");
  auto b = Value<int>(-3, "b");
  auto c = Value<int>(10, "c");
  auto e = a * b;
  auto d = e + c;
  auto f = Value<int>(-2, "f");
  auto L = d * f;
  runBackprop(L);

  // Graph graph = build_value_graph_with_ops(L);
  Graph graph = build_value_graph_with_ops(L);

  write_dot_file(graph, "value_graph.dot");

  // Manual backprop example #2: A neuron
  auto x1 = Value(2.0, "x1");
  auto x2 = Value(0.0, "x2");
  auto w1 = Value(-3.0, "w1");
  auto w2 = Value(1.0, "w2");

  auto bias = Value(6.8813735870195432, "b");
  auto x1w1 = x1 * w1;
  auto x2w2 = x2 * w2;
  auto n = x1w1 + x2w2 + bias;
  auto o = n.tanh();
  runBackprop(o);
  Graph graph2 = build_value_graph_with_ops(o);

  write_dot_file(graph2, "neuron.dot");

  return 0;
}