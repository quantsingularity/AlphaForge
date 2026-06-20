#include "TestFramework.hpp"

#include "backtesting/Backtester.hpp"
#include "backtesting/StrategyFactory.hpp"
#include "risk/RiskEngine.hpp"

#include <cmath>
#include <span>
#include <vector>

using namespace std;

using namespace alphaforge;

TEST_CASE("risk.max_drawdown_curve") {
    RiskEngine risk;
    vector<double> equity{100, 120, 90, 110, 80, 130};
    // Peak 120 then trough 80 -> drawdown (120-80)/120 = 0.3333.
    CHECK_NEAR(risk.max_drawdown_curve(span<const double>{equity}), 1.0 / 3.0, 1e-9);
}

TEST_CASE("risk.var_and_cvar_positive_losses") {
    RiskEngine risk;
    vector<double> r{-0.05, -0.02, 0.01, 0.03, -0.04, 0.02, -0.06, 0.015,
                          -0.01, 0.025};
    const double var = risk.value_at_risk(span<const double>{r}, 0.9);
    const double cvar = risk.conditional_var(span<const double>{r}, 0.9);
    CHECK(var >= 0.0);
    CHECK(cvar >= var - 1e-9);   // expected shortfall is at least as severe
}

TEST_CASE("risk.beta_of_scaled_series") {
    RiskEngine risk;
    vector<double> bench{0.01, -0.02, 0.015, -0.01, 0.02};
    vector<double> asset;
    for (double b : bench) asset.push_back(2.0 * b);  // beta should be 2
    const auto beta = risk.beta(span<const double>{asset},
                                span<const double>{bench});
    CHECK(beta.has_value());
    CHECK_NEAR(*beta, 2.0, 1e-9);
}

TEST_CASE("risk.volatility_annualizes") {
    RiskEngine risk;
    // Constant returns have zero volatility.
    vector<double> flat(50, 0.001);
    CHECK_NEAR(risk.annualized_volatility(span<const double>{flat}), 0.0, 1e-12);
}

namespace {
vector<Bar> make_bars(const vector<double>& closes) {
    vector<Bar> bars;
    int day = 1;
    for (double c : closes) {
        Bar b;
        char buf[24];
        snprintf(buf, sizeof(buf), "2023-01-%02d", (day++) % 100);
        b.date = buf;
        b.open = b.high = b.low = b.close = c;
        b.high = c * 1.01;
        b.low = c * 0.99;
        b.volume = 1000;
        bars.push_back(b);
    }
    return bars;
}
}  // namespace

TEST_CASE("backtest.buy_and_hold_matches_price_return") {
    // With zero costs, buy and hold total return equals the price return.
    vector<double> closes{100, 101, 102, 103, 104, 105, 106, 107, 108, 110};
    auto bars = make_bars(closes);
    BacktestConfig cfg;
    cfg.transaction_cost_bps = 0.0;
    cfg.slippage_bps = 0.0;
    Backtester bt(cfg);
    BuyAndHold strat;
    auto result = bt.run(strat, span<const Bar>{bars}, "TEST");
    const double price_return = closes.back() / closes.front() - 1.0;
    CHECK_NEAR(result.metrics.total_return, price_return, 1e-6);
    CHECK(result.equity_curve.size() == closes.size());
}

TEST_CASE("backtest.costs_reduce_return") {
    vector<double> closes{100, 102, 101, 103, 105, 104, 106, 108, 107, 109};
    auto bars = make_bars(closes);
    Backtester free_bt(BacktestConfig{1'000'000.0, 0.0, 0.0});
    Backtester costly_bt(BacktestConfig{1'000'000.0, 50.0, 20.0});
    auto strat = StrategyFactory::create(StrategyType::MovingAverageCrossover);
    auto a = free_bt.run(*strat, span<const Bar>{bars}, "TEST");
    auto b = costly_bt.run(*strat, span<const Bar>{bars}, "TEST");
    CHECK(b.metrics.total_return <= a.metrics.total_return + 1e-12);
}

TEST_CASE("factory.creates_all_strategies") {
    for (const auto& name : StrategyFactory::available()) {
        auto s = StrategyFactory::create(name);
        CHECK(s != nullptr);
    }
    CHECK(StrategyFactory::create("nonsense") == nullptr);
}
