#ifndef MARKETS_BINOMIAL_TREE_H_
#define MARKETS_BINOMIAL_TREE_H_

#include <Eigen/Dense>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <vector>

#include "markets/time.h"

namespace markets {

struct BasicTree {
  BasicTree(double tree_duration_years, double initial_timestep_years)
      : tree_duration_years_(tree_duration_years),
        initial_timestep_years_(initial_timestep_years) {
    // Always initialize the tree, even if it will evolve later.
    int num_timesteps =
        std::ceil(tree_duration_years_ / initial_timestep_years_) + 1;
    tree_.resize(num_timesteps, num_timesteps);
    tree_.setZero();

    total_durations_.resize(num_timesteps);
    for (int i = 0; i < num_timesteps; ++i) {
      total_durations_[i] = i * initial_timestep_years;
    }
  }

  double timestepAt(int t) const {
    // Basic error-handling: Assume the final timestep is the final word.
    if (t >= total_durations_.size() - 1) {
      return timestepAt(total_durations_.size() - 2);
    }
    return total_durations_[t + 1] - total_durations_[t];
  }

  // Stores the raw node values.
  // These can be: an underlying asset (stock or commodity), interest rates, FX
  // rates, or any such stochastic variable whose evolution we model forward in
  // time. It can also be something that is used to price derivative securities.
  // But here it doesn't matter. It's just a data structure that is better than
  // Excel. We will give it some good accessors/mutators and other behaviours.
  Eigen::MatrixXd tree_;

  // Whether we have a fixed or variable timestep, it's essential to know these
  // precisely. In the case of a fixed timestep, these will be evenly spaced.
  // But that's still useful, because we just need to do a lookup to get dt. We
  // won't need to add special-case logic.
  Eigen::VectorXd total_durations_;

  // We store the initial timestep as a consistent starting point.
  // Even if the tree is resized (i.e. because of term structure of vol changing
  // shape) it gives a reference point for how to initialise the tree.
  double initial_timestep_years_;

  double tree_duration_years_;
};

// Just some utility functions for convenience...
inline BasicTree create(std::chrono::years total_duration,
                        std::chrono::weeks timestep,
                        YearStyle style = YearStyle::k365) {
  return BasicTree(total_duration.count(),
                   timestep.count() * 7 / numDaysInYear(style));
}

inline BasicTree create(std::chrono::months total_duration,
                        std::chrono::days timestep,
                        YearStyle style = YearStyle::k365) {
  return BasicTree(total_duration.count() / 12.0,
                   timestep.count() / numDaysInYear(style));
}

class BinomialTree {
 public:
  BinomialTree(std::chrono::years total_duration,
               std::chrono::weeks timestep,
               YearStyle style = YearStyle::k365)
      : tree_duration_years_(total_duration.count()),
        timestep_years_(timestep.count() * 7 / numDaysInYear(style)) {
    int num_timesteps = std::ceil(tree_duration_years_ / timestep_years_) + 1;
    tree_.resize(num_timesteps, num_timesteps);
    tree_.setZero();
  }

  BinomialTree(std::chrono::months total_duration,
               std::chrono::days timestep,
               YearStyle style = YearStyle::k365)
      : tree_duration_years_(total_duration.count() / 12.0),
        timestep_years_(timestep.count() / numDaysInYear(style)) {
    int num_timesteps = std::ceil(tree_duration_years_ / timestep_years_) + 1;
    tree_.resize(num_timesteps, num_timesteps);
    tree_.setZero();
  }

  BinomialTree(double total_duration_years,
               double timestep_years,
               YearStyle style = YearStyle::k365)
      : tree_duration_years_(total_duration_years),
        timestep_years_(timestep_years) {
    int num_timesteps = std::ceil(tree_duration_years_ / timestep_years_) + 1;
    tree_.resize(num_timesteps, num_timesteps);
    tree_.setZero();
  }

  int numTimesteps() const { return tree_.rows(); }

  template <typename PropagatorT>
  void forwardPropagate(const PropagatorT& fwd_prop) {
    for (int t = 0; t < numTimesteps(); t++) {
      for (int i = 0; i <= t; ++i) {
        setValue(t, i, fwd_prop(*this, t, i));
      }
    }
  }

  template <typename PropagatorT>
  void backPropagate(const BinomialTree& diffusion,
                     const PropagatorT& back_prop,
                     const std::function<double(double)>& payoff_fn,
                     double expiry_years) {
    int t_final = getTimeIndexForExpiry(expiry_years);

    for (int i = t_final + 1; i < tree_.rows(); ++i) {
      tree_.row(i).setZero();
    }

    for (int i = 0; i <= t_final; ++i) {
      setValue(t_final, i, payoff_fn(diffusion.nodeValue(t_final, i)));
    }

    for (int t = t_final - 1; t >= 0; --t) {
      for (int i = t; i >= 0; --i) {
        double up = nodeValue(t + 1, i + 1);
        double down = nodeValue(t + 1, i);
        double up_prob = back_prop.getUpProbAt(timestepAt(t), t, i);
        double down_prob = 1 - up_prob;

        // TODO no discounting (yet)
        setValue(t, i, up * up_prob + down * down_prob);
      }
    }
  }

  int getTimeIndexForExpiry(double expiry_years) const {
    // for example if expiry=0.5 and timestep=1/12, then we should return 6.
    // if expiry=1/12 and timestep=1/365 then we should return 30 or 31
    // (depending on rounding convention)
    if (total_times_.empty()) {
      return std::round(expiry_years / timestep_years_);
    }

    for (int t = 0; t < total_times_.size(); ++t) {
      if (t == total_times_.size() - 1) {
        return t;
      }

      const double diff_curr = std::abs(total_times_[t] - expiry_years);
      const double diff_next = std::abs(total_times_[t + 1] - expiry_years);
      if (t == 0) {
        if (diff_curr < diff_next) {
          return t;
        }
      } else {
        if (diff_curr <= std::abs(total_times_[t - 1] - expiry_years) &&
            diff_curr <= diff_next)
          return t;
      }
    }
  }

  double sumAtTimestep(int t) const { return tree_.row(t).sum(); }

  void print() const { std::cout << tree_; }
  void printAtTime(int t) const {
    std::cout << "Time " << t << ": ";
    std::cout << tree_.row(t) << std::endl;
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
  double totalTimeAtIndex(int t) const {
    if (total_times_.empty())
      return timestep_years_ * t;
    else
      return total_times_[t];
  }

  double timestepAt(int t) const {
    if (timesteps_.empty()) {
      return timestep_years_;
    }
    return timesteps_[t];
  }

  double treeDurationYears() const { return tree_duration_years_; }

 private:
  Eigen::MatrixXd tree_;
  double tree_duration_years_;
  double timestep_years_;

  // Only used in case of time-dep vol. There is a better way to structure this.
  std::vector<double> timesteps_;
  std::vector<double> total_times_;

  void setValue(int time, int node_index, double val) {
    tree_(time, node_index) = val;
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