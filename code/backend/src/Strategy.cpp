#include "backtesting/Strategy.hpp"

#include "utils/Statistics.hpp"

#include <cmath>
#include <numeric>
#include <span>
#include <vector>

using namespace std;

namespace alphaforge {

namespace {

vector<double> closes_of(span<const Bar> bars) {
    vector<double> c;
    c.reserve(bars.size());
    for (const auto& b : bars) {
        c.push_back(b.close);
    }
    return c;
}

// Simple moving average ending at index i over `window` samples. Returns nan
// until there is enough history.
double sma_at(const vector<double>& xs, size_t i, size_t window) {
    if (window == 0 || i + 1 < window) {
        return nan("");
    }
    double sum = 0.0;
    for (size_t k = i + 1 - window; k <= i; ++k) {
        sum += xs[k];
    }
    return sum / static_cast<double>(window);
}

} // namespace

vector<double> BuyAndHold::generate_signals(span<const Bar> bars) const {
    return vector<double>(bars.size(), 1.0);
}

vector<double> MovingAverageCrossover::generate_signals(
    span<const Bar> bars) const {
    const auto closes = closes_of(bars);
    vector<double> signals(bars.size(), 0.0);
    for (size_t i = 0; i < closes.size(); ++i) {
        const double fast = sma_at(closes, i, fast_);
        const double slow = sma_at(closes, i, slow_);
        if (!isnan(fast) && !isnan(slow)) {
            signals[i] = (fast > slow) ? 1.0 : 0.0;
        }
    }
    return signals;
}

vector<double> Momentum::generate_signals(span<const Bar> bars) const {
    const auto closes = closes_of(bars);
    vector<double> signals(bars.size(), 0.0);
    for (size_t i = 0; i < closes.size(); ++i) {
        if (i >= lookback_ && closes[i - lookback_] > 0.0) {
            const double trailing = closes[i] / closes[i - lookback_] - 1.0;
            signals[i] = trailing > 0.0 ? 1.0 : 0.0;
        }
    }
    return signals;
}

vector<double> MeanReversion::generate_signals(span<const Bar> bars) const {
    const auto closes = closes_of(bars);
    vector<double> signals(bars.size(), 0.0);
    double position = 0.0;  // persists across bars: 0 flat, 1 long

    for (size_t i = 0; i < closes.size(); ++i) {
        if (i + 1 < window_) {
            signals[i] = position;
            continue;
        }
        span<const double> win{closes.data() + (i + 1 - window_), window_};
        const auto mean = stats::mean(win);
        const auto sd   = stats::stdev(win);
        if (mean && sd && *sd > 0.0) {
            const double z = (closes[i] - *mean) / *sd;
            if (position == 0.0 && z < -entry_z_) {
                position = 1.0;        // price stretched below mean: go long
            } else if (position == 1.0 && z >= -exit_z_) {
                position = 0.0;        // reverted to band: take profit
            }
        }
        signals[i] = position;
    }
    return signals;
}

} // namespace alphaforge
