#ifndef MARKETS_RATES_MARKET_H_
#define MARKETS_RATES_MARKET_H_

#include <chrono>

#include "binomial_tree.h"
#include "propagators.h"
#include "time.h"

namespace markets {

double constVolFn(double const_vol, double _) { return const_vol; }

class RatesMarket {
  RatesMarket(double spot_rate, double const_vol)
      : short_rate_diffusion_(std::chrono::months(10 * 12),
                              std::chrono::days(15),
                              YearStyle::k360),
        cir_prop_(
            spot_rate, 8.8, 0.05, std::bind_front(constVolFn(const_vol))) {
    short_rate_diffusion_.resizeWithTimeDependentVol(
        std::bind_front(constVolFn(const_vol)));
    short_rate_diffusion_.propagate(cir_prop_);

    // Compute the discounted risk-neutral probabilities at each tree node.
    // (The sum at each timestep is the discount factor).
    arrow_deb_tree_.forwardPropagate(arrow_deb_prop_);
  }

  double discountFactor(double t) const {
    int ti_left = arrow_deb_tree_.getTimeIndexForExpiry(t);
    double t_left = arrow_deb_tree_.totalTimeAtIndex(ti_left);
    double df_left = arrow_deb_tree_.sumAtTimestep(ti_left);
    if (t_left == t) {
      // The requested discount factor is exactly on a tree timestep.
      return df_left;
    }

    // Grab the next index.
    int ti_right = ti_left + 1;

    // TODO: Error check: verify that we haven't run off the end of the tree.

    // Since we are between two timesteps, we also get the
    double df_right = arrow_deb_tree_.sumAtTimestep(ti_right);

    double t_right = arrow_deb_tree_.totalTimeAtIndex(ti_right);
    // TODO: Error check: verify that t_right - t_left > 0.

    // Compute the forward rate in the current interval.
    double fwd_rate = std::log(df_left / df_right) / (t_right - t_left);

    // Assume a constant forward rate.
    double dt_residual = t - t_left;
    return df_left * std::exp(fwd_rate * dt_residual);
  }

 private:
  BinomialTree short_rate_diffusion_;
  CIRPropagator cir_prop_;

  BinomialTree arrow_deb_tree_;
  ArrowDebreauPropagator arrow_deb_prop_;
};

}  // namespace markets

#endif  // MARKETS_RATES_MARKET_H_