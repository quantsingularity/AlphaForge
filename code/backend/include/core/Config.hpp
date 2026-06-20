#pragma once

#include "common/Types.hpp"

#include <mutex>
#include <string>

using namespace std;

namespace alphaforge {

// Application wide configuration. Singleton is used here deliberately and only
// here: there is exactly one configuration for the process and it must be
// reachable from every layer without threading it through constructors. All
// accessors are guarded so config can be read from worker threads safely.
class Config {
public:
    static Config& instance();

    Config(const Config&)            = delete;
    Config& operator=(const Config&) = delete;

    // Risk free rate per annum, used by Sharpe and Sortino.
    [[nodiscard]] double risk_free_rate() const;
    void set_risk_free_rate(double r);

    // Trading periods per year, used to annualise daily statistics.
    [[nodiscard]] double periods_per_year() const;
    void set_periods_per_year(double p);

    // Round trip transaction cost in basis points applied by the backtester.
    [[nodiscard]] double transaction_cost_bps() const;
    void set_transaction_cost_bps(double bps);

    // Slippage in basis points applied to fills by the backtester.
    [[nodiscard]] double slippage_bps() const;
    void set_slippage_bps(double bps);

    // Default starting cash for new portfolios and backtests.
    [[nodiscard]] Money initial_capital() const;
    void set_initial_capital(Money c);

    [[nodiscard]] string data_directory() const;
    void set_data_directory(string dir);

private:
    Config() = default;

    mutable mutex mutex_;
    double risk_free_rate_{0.02};
    double periods_per_year_{252.0};
    double transaction_cost_bps_{5.0};
    double slippage_bps_{2.0};
    Money  initial_capital_{1'000'000.0};
    string data_directory_{"data"};
};

} // namespace alphaforge
