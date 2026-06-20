#pragma once

#include "common/Types.hpp"
#include "market/Bar.hpp"

#include <memory>
#include <span>
#include <string>
#include <vector>

using namespace std;

namespace alphaforge {

// Strategy pattern. A strategy maps a bar history to a target exposure series.
// Each element of the returned vector is the desired portfolio weight for that
// bar in the range [-1, 1], where 1 is fully long, 0 flat, -1 fully short. The
// backtester applies signal[i] as the target held into bar i+1, which avoids
// look ahead bias because the signal at i uses only information up to i.
class Strategy {
public:
    virtual ~Strategy() = default;
    [[nodiscard]] virtual string name() const = 0;
    [[nodiscard]] virtual vector<double> generate_signals(
        span<const Bar> bars) const = 0;
};

// Always fully invested. The benchmark every other strategy is measured against.
class BuyAndHold final : public Strategy {
public:
    [[nodiscard]] string name() const override { return "BuyAndHold"; }
    [[nodiscard]] vector<double> generate_signals(
        span<const Bar> bars) const override;
};

// Long when the fast simple moving average is above the slow one, flat
// otherwise. The classic trend following crossover.
class MovingAverageCrossover final : public Strategy {
public:
    MovingAverageCrossover(size_t fast = 20, size_t slow = 50)
        : fast_(fast), slow_(slow) {}
    [[nodiscard]] string name() const override { return "MovingAverageCrossover"; }
    [[nodiscard]] vector<double> generate_signals(
        span<const Bar> bars) const override;

private:
    size_t fast_;
    size_t slow_;
};

// Long when the trailing return over the lookback window is positive, flat
// otherwise. Simple time series momentum.
class Momentum final : public Strategy {
public:
    explicit Momentum(size_t lookback = 63) : lookback_(lookback) {}
    [[nodiscard]] string name() const override { return "Momentum"; }
    [[nodiscard]] vector<double> generate_signals(
        span<const Bar> bars) const override;

private:
    size_t lookback_;
};

// Mean reversion on a z score of price relative to its moving average. Goes long
// when price is sufficiently below the mean and flat (takes profit) when it
// reverts back above the upper band.
class MeanReversion final : public Strategy {
public:
    MeanReversion(size_t window = 20, double entry_z = 1.0, double exit_z = 0.0)
        : window_(window), entry_z_(entry_z), exit_z_(exit_z) {}
    [[nodiscard]] string name() const override { return "MeanReversion"; }
    [[nodiscard]] vector<double> generate_signals(
        span<const Bar> bars) const override;

private:
    size_t window_;
    double      entry_z_;
    double      exit_z_;
};

} // namespace alphaforge
