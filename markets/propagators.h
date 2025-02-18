#ifndef MARKETS_PROPAGATORS_H_
#define MARKETS_PROPAGATORS_H_

#include <memory>

#include "markets/binomial_tree.h"

namespace markets {

struct CRRPropagator {
  CRRPropagator(double expected_drift, double annualized_vol, double spot_price)
      : expected_drift_(expected_drift),
        annualized_vol_(annualized_vol),
        spot_price_(spot_price) {}

  CRRPropagator(double expected_drift,
                double spot_price,
                const TimeDepVolFn& vol_fn)
      : expected_drift_(expected_drift),
        spot_price_(spot_price),
        vol_fn_(std::make_unique<const TimeDepVolFn>(vol_fn)) {}

  double getVol(double t) const {
    if (isVolConst()) {
      return annualized_vol_;
    }
    return (*vol_fn_)(t);
  }

  bool isVolConst() const { return vol_fn_ == nullptr; }

  double operator()(const BinomialTree& tree, int t, int i) const {
    if (t == 0) return spot_price_;
    double curr_time = tree.totalTimeAtIndex(t);
    double dt = tree.timestepAt(t);
    double u = getVol(curr_time) * std::sqrt(dt);

    if (i == 0) {
      double d = -u;
      return tree.nodeValue(t - 1, 0) * std::exp(d);
    }

    return tree.nodeValue(t - 1, i - 1) * std::exp(u);
  }

  double getUpProbAt(double dt, int t, int i) const {
    // The "modeled" probability is 0.5 + 0.5 * (expected_drift_ /
    // annualized_vol_) * std::sqrt(dt); However here for asset pricing we use
    // the risk-neutral:
    double u = annualized_vol_ * std::sqrt(dt);
    double d = -u;
    double r_temp = 0.0;  // TODO add rate
    return (std::exp(r_temp * dt) - std::exp(d)) / (std::exp(u) - std::exp(d));
  }

  // void updateVol(double vol) { annualized_vol_ = vol; }
  void updateSpot(double spot) { spot_price_ = spot; }

  double expected_drift_;
  double annualized_vol_;
  double spot_price_;
  std::unique_ptr<const TimeDepVolFn> vol_fn_;
};

struct JarrowRuddPropagator {
  JarrowRuddPropagator(double expected_drift,
                       double annualized_vol,
                       double spot_price)
      : expected_drift_(expected_drift),
        annualized_vol_(annualized_vol),
        spot_price_(spot_price) {}

  double operator()(const BinomialTree& tree, int t, int i) const {
    if (t == 0) return spot_price_;
    double dt = tree.timestepAt(t);

    if (i == 0) {
      double d = expected_drift_ * dt - annualized_vol_ * std::sqrt(dt);
      return tree.nodeValue(t - 1, 0) * std::exp(d);
    } else {
      double u = expected_drift_ * dt + annualized_vol_ * std::sqrt(dt);
      return tree.nodeValue(t - 1, i - 1) * std::exp(u);
    }
  }

  double getUpProbAt(double dt, int t, int i) const {
    // The "modeled" probability is 0.5.
    // However, here for asset pricing we use the risk-neutral:
    double u = expected_drift_ * dt + annualized_vol_ * std::sqrt(dt);
    double d = expected_drift_ * dt - annualized_vol_ * std::sqrt(dt);
    double r_temp = 0.0;  // TODO add rate
    return (std::exp(r_temp * dt) - std::exp(d)) / (std::exp(u) - std::exp(d));
  }

  void updateVol(double vol) { annualized_vol_ = vol; }
  void updateSpot(double spot) { spot_price_ = spot; }

  double expected_drift_;
  double annualized_vol_;
  double spot_price_;
};

/*
b (Long-Term Mean):  This represents the level to which the short-rate tends to
revert in the long run.  As you correctly guessed, it should be in the same
units as interest rates (e.g., decimal or percentage).  So, if you expect rates
to average around 5%, b would be 0.05 (or 5.0 if you're working with percentages
directly).

  a (Mean Reversion Speed): This parameter controls how quickly the short-rate
reverts to the long-term mean.  It's a speed or rate of adjustment.  The units
of a are inverse time.  If your time step in the binomial tree is one year, the
units of a are 1/year.  
  */
struct CIRPropagator {
  CIRPropagator(double spot_rate,
                double a,  // mean-reversion speed
                double b,  // long-term mean,
                const TimeDepVolFn& vol_fn)
      : spot_rate_(spot_rate),
        a_(a),
        b_(b),
        vol_fn_(std::make_unique<const TimeDepVolFn>(vol_fn)) {}

  double operator()(const BinomialTree& tree, int t, int i) const {
    if (t == 0) return spot_rate_;

    double curr_time = tree.totalTimeAtIndex(t);
    double dt = tree.timestepAt(t);
    double vol = (*vol_fn_)(curr_time);

    if (i == 0) {
      double r_prev = tree.nodeValue(t - 1, 0);
      return r_prev * std::exp(-a_ * dt) + b_ * (1 - exp(-a_ * dt)) -
             vol * sqrt(dt * r_prev);
    }
    double r_prev = tree.nodeValue(t - 1, i - 1);
    return r_prev * std::exp(-a_ * dt) + b_ * (1 - exp(-a_ * dt)) +
           vol * sqrt(dt * r_prev);  // Up step
  }

  double getUpProbAt(double dt, int t, int i) const {
    return 0.5;  // placeholder.
  }

  void updateSpot(double spot) { spot_rate_ = spot; }
  void updateVol(double vol) {}  // placeholder

  double spot_rate_;
  double a_;
  double b_;
  std::unique_ptr<const TimeDepVolFn> vol_fn_;
};

}  // namespace markets

#endif  // MARKETS_PROPAGATORS_H_