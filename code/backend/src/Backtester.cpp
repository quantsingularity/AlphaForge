#include "backtesting/Backtester.hpp"

#include "core/Config.hpp"
#include "utils/Statistics.hpp"

#include <algorithm>
#include <cmath>
#include <span>
#include <vector>

using namespace std;

namespace alphaforge {

vector<EquityPoint> Backtester::simulate(span<const double> signals,
                                              span<const Bar> bars,
                                              size_t& trades_out) const {
    vector<EquityPoint> curve;
    trades_out = 0;
    if (bars.size() < 2 || signals.size() != bars.size()) {
        return curve;
    }

    const double cost_rate =
        (config_.transaction_cost_bps + config_.slippage_bps) / 10'000.0;

    double equity = config_.initial_capital;
    double prev_weight = 0.0;
    constexpr double kEps = 1e-9;

    curve.push_back(EquityPoint{string{bars[0].date}, equity, signals[0]});

    for (size_t i = 1; i < bars.size(); ++i) {
        const double new_weight = signals[i - 1];  // decided with info up to i-1
        const double turnover   = abs(new_weight - prev_weight);
        const double cost        = turnover * cost_rate;

        const double asset_return =
            bars[i - 1].close != 0.0 ? bars[i].close / bars[i - 1].close - 1.0 : 0.0;
        const double period_return = new_weight * asset_return - cost;

        equity *= (1.0 + period_return);
        if (turnover > kEps) {
            ++trades_out;
        }
        prev_weight = new_weight;

        curve.push_back(EquityPoint{string{bars[i].date}, equity, new_weight});
    }
    return curve;
}

BacktestMetrics Backtester::summarize(const vector<EquityPoint>& curve,
                                      const vector<double>& returns,
                                      size_t trades) const {
    BacktestMetrics m;
    m.num_trades = trades;
    if (curve.size() < 2) {
        return m;
    }

    vector<double> equity;
    equity.reserve(curve.size());
    for (const auto& p : curve) {
        equity.push_back(p.equity);
    }

    m.total_return = equity.front() != 0.0 ? equity.back() / equity.front() - 1.0 : 0.0;

    const double periods = Config::instance().periods_per_year();
    const double years = static_cast<double>(returns.size()) / periods;
    if (years > 0.0 && equity.front() > 0.0 && equity.back() > 0.0) {
        m.cagr = pow(equity.back() / equity.front(), 1.0 / years) - 1.0;
    }

    const span<const double> r{returns};
    m.annualized_volatility = risk_.annualized_volatility(r);
    m.sharpe_ratio          = risk_.sharpe_ratio(r);
    m.sortino_ratio         = risk_.sortino_ratio(r);
    m.max_drawdown          = risk_.max_drawdown_curve(span<const double>{equity});

    size_t invested_days = 0, win_days = 0;
    double gross_gain = 0.0, gross_loss = 0.0;
    for (const double ret : returns) {
        if (abs(ret) > 1e-12) {
            ++invested_days;
            if (ret > 0.0) {
                ++win_days;
                gross_gain += ret;
            } else {
                gross_loss += -ret;
            }
        }
    }
    m.win_rate = invested_days > 0
                     ? static_cast<double>(win_days) / static_cast<double>(invested_days)
                     : 0.0;
    if (gross_loss > 0.0) {
        m.profit_factor = min(99.99, gross_gain / gross_loss);
    } else if (gross_gain > 0.0) {
        m.profit_factor = 99.99;  // capped: no losing periods in the sample
    }
    return m;
}

BacktestResult Backtester::run(const Strategy& strategy, span<const Bar> bars,
                               const Symbol& symbol) const {
    BacktestResult result;
    result.strategy = strategy.name();
    result.symbol   = symbol;

    if (bars.size() < 2) {
        return result;
    }

    const auto signals = strategy.generate_signals(bars);

    size_t trades = 0;
    result.equity_curve = simulate(span<const double>{signals}, bars, trades);

    result.returns.reserve(result.equity_curve.size());
    for (size_t i = 1; i < result.equity_curve.size(); ++i) {
        const double prev = result.equity_curve[i - 1].equity;
        result.returns.push_back(prev != 0.0 ? result.equity_curve[i].equity / prev - 1.0
                                             : 0.0);
    }
    result.metrics = summarize(result.equity_curve, result.returns, trades);

    // Buy and hold benchmark over the identical window.
    const vector<double> bh_signals(bars.size(), 1.0);
    size_t bh_trades = 0;
    const auto bh_curve = simulate(span<const double>{bh_signals}, bars, bh_trades);
    vector<double> bh_returns;
    bh_returns.reserve(bh_curve.size());
    for (size_t i = 1; i < bh_curve.size(); ++i) {
        const double prev = bh_curve[i - 1].equity;
        bh_returns.push_back(prev != 0.0 ? bh_curve[i].equity / prev - 1.0 : 0.0);
    }
    result.benchmark = summarize(bh_curve, bh_returns, bh_trades);

    return result;
}

} // namespace alphaforge
