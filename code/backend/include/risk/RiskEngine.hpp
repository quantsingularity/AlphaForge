#pragma once

#include "common/Types.hpp"
#include "utils/ThreadPool.hpp"

#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

namespace alphaforge {

// Bundle of risk metrics computed from a return series.
struct RiskMetrics {
    double annualized_volatility{0};
    double sharpe_ratio{0};
    double sortino_ratio{0};
    double max_drawdown{0};        // reported as a positive fraction, e.g. 0.18
    double value_at_risk{0};       // historical VaR at the chosen confidence
    double conditional_var{0};     // expected shortfall beyond VaR
    optional<double> beta;    // present only when a benchmark is supplied
};

// Stateless risk calculator. Each method takes a return series (period over
// period simple returns) and uses the configured periods per year and risk
// free rate for annualisation. VaR and CVaR use the historical method.
class RiskEngine {
public:
    RiskEngine() = default;

    [[nodiscard]] double annualized_volatility(span<const double> returns) const;
    [[nodiscard]] double sharpe_ratio(span<const double> returns) const;
    [[nodiscard]] double sortino_ratio(span<const double> returns) const;

    // Maximum drawdown of the equity curve implied by compounding returns,
    // returned as a positive fraction of peak equity.
    [[nodiscard]] double max_drawdown(span<const double> returns) const;

    // Maximum drawdown computed directly from an equity (or price) curve.
    [[nodiscard]] double max_drawdown_curve(span<const double> equity) const;

    // Beta of the asset returns against benchmark returns.
    [[nodiscard]] optional<double> beta(span<const double> asset,
                                             span<const double> benchmark) const;

    // Historical VaR at the given confidence (e.g. 0.95). Returned as a positive
    // loss fraction: 0.03 means a 3 percent loss threshold.
    [[nodiscard]] double value_at_risk(span<const double> returns,
                                       double confidence = 0.95) const;

    // Conditional VaR / expected shortfall at the given confidence.
    [[nodiscard]] double conditional_var(span<const double> returns,
                                         double confidence = 0.95) const;

    // Compute the full bundle in one pass.
    [[nodiscard]] RiskMetrics compute(span<const double> returns,
                                      optional<span<const double>> benchmark = nullopt,
                                      double confidence = 0.95) const;

    // Compute metrics for many symbols in parallel using a thread pool. The map
    // values are return series keyed by symbol.
    [[nodiscard]] unordered_map<Symbol, RiskMetrics> compute_batch(
        const unordered_map<Symbol, vector<double>>& returns_by_symbol,
        ThreadPool& pool,
        double confidence = 0.95) const;
};

} // namespace alphaforge
