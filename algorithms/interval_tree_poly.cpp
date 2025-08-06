#include "interval_region.h"
#include "interval_tree.h"
#include "polyscope/curve_network.h"
#include "polyscope/polyscope.h"

// Global
IntervalTree* g_tree = nullptr;
// Global or static variable for the query point
float query_point = 0.5f;

// Helper to collect all intervals from tree (recursive)
void collectIntervals(const IntervalTree& tree,
                      std::vector<Interval>& intervals) {
  intervals.insert(
      intervals.end(), tree.mid_left_sort.begin(), tree.mid_left_sort.end());
  if (tree.left) collectIntervals(*tree.left, intervals);
  if (tree.right) collectIntervals(*tree.right, intervals);
}

void visualizeIntervalTree(const IntervalTree& tree, float query_point) {
  std::vector<std::array<double, 3>> nodes;
  std::vector<std::array<size_t, 2>> edges;

  // Collect all intervals
  std::vector<Interval> all_intervals;
  collectIntervals(tree, all_intervals);

  // For each interval, create a horizontal line segment
  size_t idx = 0;
  for (const auto& interval : all_intervals) {
    nodes.push_back({interval.x_min, interval.y, 0});
    nodes.push_back({interval.x_max, interval.y, 0});
    edges.push_back({idx, idx + 1});
    idx += 2;
  }

  auto* network = polyscope::registerCurveNetwork("intervals", nodes, edges);
  network->setRadius(0.0001);

  // Color based on query intersection
  std::vector<std::array<double, 3>> colors;
  for (const auto& interval : all_intervals) {
    auto color = interval.contains(query_point)
                     ? std::array<double, 3>{0, 1, 0}         // Green if hit
                     : std::array<double, 3>{0.5, 0.5, 0.5};  // Gray if not
    colors.push_back(color);  // ONE color per edge/interval
  }
  network->addEdgeColorQuantity("query_hits", colors)->setEnabled(true);

  // Add vertical line for query - simpler approach
  std::vector<std::array<double, 3>> query_nodes = {{query_point, 0, 0},
                                                    {query_point, 1, 0}};
  std::vector<std::array<size_t, 2>> query_edge = {{0, 1}};
  auto* queryline =
      polyscope::registerCurveNetwork("query", query_nodes, query_edge);
  queryline->setRadius(0.002);
  queryline->setColor({1, 0, 0.5});
}

void myCallback() {
  // Add UI elements
  if (ImGui::SliderFloat("Query Point", &query_point, 0.0f, 1.0f)) {
    // Slider was moved, update visualization
    // You'll need access to your tree here - make it global or pass differently
    visualizeIntervalTree(*g_tree, query_point);
  }

  // Add some stats
  ImGui::Text("Query at: %.3f", query_point);
  // Add more stats here
}

int main() {
  polyscope::init();

  // Create your interval tree
  auto tree = initRandomIntervalTree(300000);
  g_tree = &tree;

  static float query_point = 0.5f;
  visualizeIntervalTree(tree, query_point);

  // Set the user callback
  polyscope::state::userCallback = myCallback;

  // Set up 2D view
  polyscope::view::style = polyscope::view::NavigateStyle::Planar;
  polyscope::show();

  return 0;
}