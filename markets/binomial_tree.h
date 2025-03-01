#ifndef MARKETS_BINOMIAL_TREE_H_
#define MARKETS_BINOMIAL_TREE_H_

#include <Eigen/Dense>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <vector>

#include "markets/time.h"
#include "markets/volatility.h"

namespace markets {

class BinomialTree {
 public:
  BinomialTree(double total_duration_years, double timestep_years)
      : tree_duration_years_(total_duration_years),
        timestep_years_(timestep_years) {
    int num_timesteps = std::ceil(tree_duration_years_ / timestep_years_) + 1;
    tree_.resize(num_timesteps, num_timesteps);
    tree_.setZero();
  }

  // Helper factory functions using the chrono library.
  static BinomialTree create(std::chrono::years total_duration,
                             std::chrono::weeks timestep,
                             YearStyle style = YearStyle::k365) {
    return BinomialTree(total_duration.count(),
                        timestep.count() * 7 / numDaysInYear(style));
  }
  static BinomialTree create(std::chrono::months total_duration,
                             std::chrono::days timestep,
                             YearStyle style = YearStyle::k365) {
    return BinomialTree(total_duration.count() / 12.0,
                        timestep.count() / numDaysInYear(style));
  }

  static BinomialTree createFrom(const BinomialTree& underlying) {
    BinomialTree derived = underlying;
    derived.tree_.setZero();
    return derived;
  }

  int numTimesteps() const { return tree_.rows(); }

  void setInitValue(double val) { setValue(0, 0, val); }

  template <typename PropagatorT, typename VolatilityT>
  void forwardPropagate(const PropagatorT& fwd_prop, const VolatilityT& vol) {
    resizeWithTimeDependentVol(vol);
    for (int t = 0; t < numTimesteps(); t++) {
      for (int i = 0; i <= t; ++i) {
        setValue(t, i, fwd_prop(*this, vol, t, i));
      }
    }
  }

  // Equation 13.23a (Derman) for the risk-neutral, no-arbitrage up probability.
  double getUpProbAt(int t, int i) const {
    // Hack:
    if (t >= timegrid_.size() - 1) {
      t = timegrid_.size() - 2;
    }

    double curr = nodeValue(t, i);
    double up_ratio = nodeValue(t + 1, i + 1) / curr;
    double down_ratio = nodeValue(t + 1, i) / curr;
    double dt = timegrid_.dt(t);
    double r_temp = 0.0;  // TODO add rate
    return (std::exp(r_temp * dt) - down_ratio) / (up_ratio - down_ratio);
  }

  void backPropagate(const BinomialTree& diffusion,
                     const std::function<double(double)>& payoff_fn,
                     double expiry_years) {
    int t_final = timegrid_.getTimeIndexForExpiry(expiry_years);

    for (int i = t_final + 1; i < tree_.rows(); ++i) {
      tree_.row(i).setZero();
    }

    // Set the payoff at each scenario on the maturity date.
    for (int i = 0; i <= t_final; ++i) {
      setValue(t_final, i, payoff_fn(diffusion.nodeValue(t_final, i)));
    }

    // Back-propagation.
    for (int t = t_final - 1; t >= 0; --t) {
      // std::cout << "At timeindex:" << t << std::endl;
      for (int i = 0; i <= t; ++i) {
        double up = nodeValue(t + 1, i + 1);
        double down = nodeValue(t + 1, i);
        double up_prob = diffusion.getUpProbAt(t, i);
        double down_prob = 1 - up_prob;

        /*
        std::cout << "    up:" << up << "  down:" << down
                  << "  up_prob:" << up_prob << "  down_prob:" << down_prob
                  << std::endl;
*/

        // TODO no discounting (yet)
        setValue(t, i, up * up_prob + down * down_prob);
      }
    }
  }

  double sumAtTimestep(int t) const { return tree_.row(t).sum(); }

  void print() const { std::cout << tree_; }
  void printAtTime(int t) const {
    std::cout << "Time " << t << ": ";
    std::cout << tree_.row(t) << std::endl;
  }
  void printUpTo(int ti) const {
    for (int i = 0; i < ti; ++i) {
      std::cout << "t:" << i << " ::  " << tree_.row(i).head(i + 1)
                << std::endl;
    }
  }

  void printProbabilitiesUpTo(int ti) const {
    for (int t = 0; t < ti; ++t) {
      std::cout << "t:" << t << " q:";
      for (int i = 0; i <= t; ++i) {
        std::cout << " " << getUpProbAt(t, i);
      }
      std::cout << std::endl;
    }
  }

  double nodeValue(int time, int node_index) const {
    return tree_(time, node_index);
  }

  bool isTreeEmptyAt(int t) const {
    // current assumption: if an entire row is 0, nothing after it can be
    // populated.
    return tree_.row(t).isZero(0);
  }

  double exactTimestepInYears() const { return timestep_years_; }
  double totalTimeAtIndex(int ti) const { return timegrid_.time(ti); }
  double timestepAt(int ti) const { return timegrid_.dt(ti); }

  double treeDurationYears() const { return tree_duration_years_; }

 private:
  Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> tree_;
  double tree_duration_years_;
  double timestep_years_;

  Timegrid timegrid_;

  void setValue(int time, int node_index, double val) {
    tree_(time, node_index) = val;
  }

  template <typename VolSurfaceT>
  void resizeWithTimeDependentVol(const Volatility<VolSurfaceT>& volfn) {
    timegrid_ = volfn.generateTimegrid(tree_duration_years_, timestep_years_);
    tree_.resize(timegrid_.size(), timegrid_.size());
    tree_.setZero();
  }
};

inline std::vector<Eigen::Vector2d> getNodes(const BinomialTree& tree) {
  std::vector<Eigen::Vector2d> nodes;
  for (int t = 0; t < tree.numTimesteps(); ++t) {
    if (tree.isTreeEmptyAt(t)) {
      break;
    }
    for (int i = 0; i <= t; ++i) {
      nodes.emplace_back(
          Eigen::Vector2d{tree.totalTimeAtIndex(t), tree.nodeValue(t, i)});
    }
  }
  return nodes;
}

struct TreeRenderData {
  std::vector<double> x_coords, y_coords, edge_x_coords, edge_y_coords;
};

TreeRenderData getTreeRenderData(const BinomialTree& tree) {
  TreeRenderData r;
  const auto nodes = getNodes(tree);
  for (const auto& node : nodes) {
    r.x_coords.push_back(node.x());
    r.y_coords.push_back(node.y());
  }

  int cumul_start_index = 0;
  for (int t = 0; t < tree.numTimesteps(); ++t) {
    for (int i = 0; i <= t; ++i) {
      if (t < tree.numTimesteps()) {
        size_t parentIndex = cumul_start_index + i;
        size_t child1Index = cumul_start_index + t + i + 0;
        size_t child2Index = cumul_start_index + t + i + 1;

        if (child1Index < nodes.size()) {
          // Add coordinates for the segment
          r.edge_x_coords.push_back(nodes[parentIndex].x());
          r.edge_y_coords.push_back(nodes[parentIndex].y());
          r.edge_x_coords.push_back(nodes[child1Index].x());
          r.edge_y_coords.push_back(nodes[child1Index].y());
        }
        if (child2Index < nodes.size()) {
          // Add coordinates for the segment
          r.edge_x_coords.push_back(nodes[parentIndex].x());
          r.edge_y_coords.push_back(nodes[parentIndex].y());
          r.edge_x_coords.push_back(nodes[child2Index].x());
          r.edge_y_coords.push_back(nodes[child2Index].y());
        }
      }
    }
    cumul_start_index += t;
  }
  return r;
}

class AssetTree {};

double call_payoff(double strike, double val) {
  return std::max(0.0, val - strike);
}

double put_payoff(double strike, double val) {
  return std::max(0.0, strike - val);
}

double digital_payoff(double strike, double val) {
  double dist_from_strike = std::abs(strike - val);
  if (dist_from_strike / strike < 0.05) {  // 5% on either side.
    return 1.0;
  }
  return 0;
}

}  // namespace markets

#endif  // MARKETS_BINOMIAL_TREE_H_