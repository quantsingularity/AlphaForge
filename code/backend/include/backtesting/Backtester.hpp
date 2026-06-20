#pragma once

#include "backtesting/Strategy.hpp"
#include "common/Types.hpp"
#include "market/Bar.hpp"
#include "risk/RiskEngine.hpp"

#include <span>
#include <string>
#include <vector>

using namespace std;

namespace alphaforge {

// Parameters for a single backtest run. Costs and slippage are expressed in
// basis points and applied to the notional traded on each rebalance.
struct BacktestConfig {
    Money  initial_capital{1'000'000.0};
    double transaction_cost_bps{5.0};
    double slippage_bps{2.0};
};

// One point on the simulated equity curve.
struct EquityPoint {
    string date;
    double      equity{0};
    double      target_weight{0};   // strategy target held into the next bar
};

// Headline performance metrics for a completed run.
struct BacktestMetrics {
    double total_return{0};
    double cagr{0};
    double annualized_volatility{0};
    double sharpe_ratio{0};
    double sortino_ratio{0};
    double max_drawdown{0};
    double win_rate{0};         // fraction of positive return days while invested
    double profit_factor{0};    // gross gains divided by gross losses
    size_t num_trades{0};  // rebalances that changed exposure
};

// Full result of a backtest: the equity curve, daily returns and metrics, plus
// the same metrics for a buy and hold benchmark over the identical window.
struct BacktestResult {
    string                 strategy;
    string                 symbol;
    vector<EquityPoint>    equity_curve;
    vector<double>         returns;
    BacktestMetrics             metrics;
    BacktestMetrics             benchmark;   // buy and hold over the same bars
};

// Event driven backtester. Iterates the bar series once, applying the strategy
// target with realistic frictions, marking equity each step. Metrics are
// derived from the resulting return series via the RiskEngine.
class Backtester {
public:
    explicit Backtester(BacktestConfig config = {}) : config_(config) {}

    [[nodiscard]] BacktestResult run(const Strategy& strategy,
                                     span<const Bar> bars,
                                     const Symbol& symbol) const;

private:
    [[nodiscard]] vector<EquityPoint> simulate(
        span<const double> signals, span<const Bar> bars,
        size_t& trades_out) const;

    [[nodiscard]] BacktestMetrics summarize(
        const vector<EquityPoint>& curve,
        const vector<double>& returns,
        size_t trades) const;

    BacktestConfig config_;
    RiskEngine     risk_;
};

} // namespace alphaforge
