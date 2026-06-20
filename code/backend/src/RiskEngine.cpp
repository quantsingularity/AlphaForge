#include "risk/RiskEngine.hpp"

#include "core/Config.hpp"
#include "utils/Statistics.hpp"

#include <algorithm>
#include <cmath>
#include <future>
#include <vector>

using namespace std;

namespace alphaforge {

double RiskEngine::annualized_volatility(span<const double> returns) const {
    const auto sd = stats::stdev(returns);
    if (!sd) {
        return 0.0;
    }
    const double periods = Config::instance().periods_per_year();
    return *sd * sqrt(periods);
}

double RiskEngine::sharpe_ratio(span<const double> returns) const {
    const auto m  = stats::mean(returns);
    const auto sd = stats::stdev(returns);
    if (!m || !sd || *sd == 0.0) {
        return 0.0;
    }
    const double periods = Config::instance().periods_per_year();
    const double rf_per   = Config::instance().risk_free_rate() / periods;
    const double excess   = *m - rf_per;
    return (excess / *sd) * sqrt(periods);
}

double RiskEngine::sortino_ratio(span<const double> returns) const {
    const auto m = stats::mean(returns);
    if (!m) {
        return 0.0;
    }
    const double periods = Config::instance().periods_per_year();
    const double mar      = Config::instance().risk_free_rate() / periods;
    const auto dd = stats::downside_deviation(returns, mar);
    if (!dd || *dd == 0.0) {
        return 0.0;
    }
    const double excess = *m - mar;
    return (excess / *dd) * sqrt(periods);
}

double RiskEngine::max_drawdown(span<const double> returns) const {
    if (returns.empty()) {
        return 0.0;
    }
    vector<double> equity;
    equity.reserve(returns.size() + 1);
    double level = 1.0;
    equity.push_back(level);
    for (const double r : returns) {
        level *= (1.0 + r);
        equity.push_back(level);
    }
    return max_drawdown_curve(equity);
}

double RiskEngine::max_drawdown_curve(span<const double> equity) const {
    if (equity.empty()) {
        return 0.0;
    }
    double peak = equity.front();
    double max_dd = 0.0;
    for (const double v : equity) {
        peak = max(peak, v);
        if (peak > 0.0) {
            const double dd = (peak - v) / peak;
            max_dd = max(max_dd, dd);
        }
    }
    return max_dd;
}

optional<double> RiskEngine::beta(span<const double> asset,
                                       span<const double> benchmark) const {
    const auto cov = stats::covariance(asset, benchmark);
    const auto var = stats::variance(benchmark);
    if (!cov || !var || *var == 0.0) {
        return nullopt;
    }
    return *cov / *var;
}

double RiskEngine::value_at_risk(span<const double> returns,
                                 double confidence) const {
    if (returns.empty()) {
        return 0.0;
    }
    const double q = 1.0 - confidence;
    const auto cutoff = stats::percentile(returns, q);
    if (!cutoff) {
        return 0.0;
    }
    // Report as a positive loss magnitude.
    return max(0.0, -*cutoff);
}

double RiskEngine::conditional_var(span<const double> returns,
                                   double confidence) const {
    if (returns.empty()) {
        return 0.0;
    }
    const double q = 1.0 - confidence;
    const auto cutoff = stats::percentile(returns, q);
    if (!cutoff) {
        return 0.0;
    }
    double sum = 0.0;
    size_t count = 0;
    for (const double r : returns) {
        if (r <= *cutoff) {
            sum += r;
            ++count;
        }
    }
    if (count == 0) {
        return max(0.0, -*cutoff);
    }
    return max(0.0, -(sum / static_cast<double>(count)));
}

RiskMetrics RiskEngine::compute(span<const double> returns,
                                optional<span<const double>> benchmark,
                                double confidence) const {
    RiskMetrics m;
    m.annualized_volatility = annualized_volatility(returns);
    m.sharpe_ratio          = sharpe_ratio(returns);
    m.sortino_ratio         = sortino_ratio(returns);
    m.max_drawdown          = max_drawdown(returns);
    m.value_at_risk         = value_at_risk(returns, confidence);
    m.conditional_var       = conditional_var(returns, confidence);
    if (benchmark) {
        m.beta = beta(returns, *benchmark);
    }
    return m;
}

unordered_map<Symbol, RiskMetrics> RiskEngine::compute_batch(
    const unordered_map<Symbol, vector<double>>& returns_by_symbol,
    ThreadPool& pool, double confidence) const {

    vector<pair<Symbol, future<RiskMetrics>>> futures;
    futures.reserve(returns_by_symbol.size());

    for (const auto& [symbol, returns] : returns_by_symbol) {
        futures.emplace_back(
            symbol,
            pool.submit(
                [this, confidence](const vector<double>& r) {
                    return this->compute(span<const double>{r}, nullopt,
                                         confidence);
                },
                returns));
    }

    unordered_map<Symbol, RiskMetrics> out;
    out.reserve(futures.size());
    for (auto& [symbol, fut] : futures) {
        out.emplace(symbol, fut.get());
    }
    return out;
}

} // namespace alphaforge
