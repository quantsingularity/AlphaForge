#pragma once

#include "analytics/AnalyticsEngine.hpp"
#include "backtesting/Backtester.hpp"
#include "backtesting/StrategyFactory.hpp"
#include "core/Config.hpp"
#include "execution/OrderManager.hpp"
#include "market/MarketDataEngine.hpp"
#include "portfolio/Portfolio.hpp"
#include "risk/RiskEngine.hpp"
#include "utils/ThreadPool.hpp"

#include <memory>
#include <mutex>
#include <string>

using namespace std;

namespace alphaforge {

// Aggregates the long lived engines behind a single facade so the CLI and the
// REST server share one wiring. Mutations that touch the portfolio and order
// book are serialised through trade_mutex.
class Application {
public:
    Application();

    // Load every CSV in the configured data directory.
    size_t bootstrap();

    MarketDataEngine& market() { return *market_; }
    Portfolio&        portfolio() { return *portfolio_; }
    OrderManager&     orders() { return *orders_; }
    RiskEngine&       risk() { return risk_; }
    AnalyticsEngine&  analytics() { return analytics_; }
    ThreadPool&       pool() { return pool_; }
    mutex&       trade_mutex() { return trade_mutex_; }

    [[nodiscard]] static string version() { return "1.0.0"; }

private:
    shared_ptr<MarketDataEngine> market_;
    shared_ptr<Portfolio>        portfolio_;
    unique_ptr<OrderManager>     orders_;
    RiskEngine                        risk_;
    AnalyticsEngine                   analytics_;
    ThreadPool                        pool_;
    mutex                        trade_mutex_;
};

} // namespace alphaforge
