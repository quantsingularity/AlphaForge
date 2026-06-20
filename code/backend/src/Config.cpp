#include "core/Config.hpp"

using namespace std;

namespace alphaforge {

Config& Config::instance() {
    static Config config;
    return config;
}

double Config::risk_free_rate() const {
    scoped_lock lock(mutex_);
    return risk_free_rate_;
}
void Config::set_risk_free_rate(double r) {
    scoped_lock lock(mutex_);
    risk_free_rate_ = r;
}

double Config::periods_per_year() const {
    scoped_lock lock(mutex_);
    return periods_per_year_;
}
void Config::set_periods_per_year(double p) {
    scoped_lock lock(mutex_);
    periods_per_year_ = p;
}

double Config::transaction_cost_bps() const {
    scoped_lock lock(mutex_);
    return transaction_cost_bps_;
}
void Config::set_transaction_cost_bps(double bps) {
    scoped_lock lock(mutex_);
    transaction_cost_bps_ = bps;
}

double Config::slippage_bps() const {
    scoped_lock lock(mutex_);
    return slippage_bps_;
}
void Config::set_slippage_bps(double bps) {
    scoped_lock lock(mutex_);
    slippage_bps_ = bps;
}

Money Config::initial_capital() const {
    scoped_lock lock(mutex_);
    return initial_capital_;
}
void Config::set_initial_capital(Money c) {
    scoped_lock lock(mutex_);
    initial_capital_ = c;
}

string Config::data_directory() const {
    scoped_lock lock(mutex_);
    return data_directory_;
}
void Config::set_data_directory(string dir) {
    scoped_lock lock(mutex_);
    data_directory_ = move(dir);
}

} // namespace alphaforge
