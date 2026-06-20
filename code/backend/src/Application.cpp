#include "core/Application.hpp"

#include "utils/Logger.hpp"

using namespace std;

namespace alphaforge {

Application::Application()
    : market_(make_shared<MarketDataEngine>()),
      portfolio_(make_shared<Portfolio>(Config::instance().initial_capital())),
      orders_(make_unique<OrderManager>(*portfolio_)) {
    // The portfolio observes market data so its marks track the latest bar.
    market_->subscribe(weak_ptr<IMarketDataObserver>(portfolio_));
}

size_t Application::bootstrap() {
    const auto dir = Config::instance().data_directory();
    auto reports = market_->load_directory(dir);
    size_t total = 0;
    for (const auto& [symbol, report] : reports) {
        total += report.accepted;
        Logger::instance().info("loaded " + symbol + ": " +
                                to_string(report.accepted) + " bars, " +
                                to_string(report.rejected) + " rejected");
        // Seed marks with the latest close so valuation works before any replay.
        if (auto last = market_->latest_bar(symbol)) {
            portfolio_->set_mark(symbol, last->close);
        }
    }
    return total;
}

} // namespace alphaforge
