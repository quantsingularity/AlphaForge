#pragma once

#include "common/Types.hpp"
#include "market/Bar.hpp"

#include <span>
#include <string>
#include <vector>

using namespace std;

namespace alphaforge {

// A labelled point in a derived series (for example a monthly return labelled
// "2023-04", or a rolling Sharpe labelled by the window end date).
struct SeriesPoint {
    string label;
    double      value{0};
};

// Analytics over a price or bar series. Frequency aggregation (weekly, monthly)
// groups by ISO date prefixes so it respects the real calendar rather than a
// naive fixed stride. Rolling metrics use a sliding window over daily returns.
class AnalyticsEngine {
public:
    AnalyticsEngine() = default;

    [[nodiscard]] vector<double> daily_returns(span<const double> closes) const;

    // Calendar grouped returns. Weekly groups by ISO year and week derived from
    // the date; monthly groups by the YYYY-MM prefix. Each point is the
    // compounded return within the group.
    [[nodiscard]] vector<SeriesPoint> weekly_returns(span<const Bar> bars) const;
    [[nodiscard]] vector<SeriesPoint> monthly_returns(span<const Bar> bars) const;

    // Compound annual growth rate from first to last close over the elapsed
    // number of periods, annualised by periods per year.
    [[nodiscard]] double annualized_return(span<const double> closes) const;

    // Rolling annualised volatility of daily returns over a window.
    [[nodiscard]] vector<SeriesPoint> rolling_volatility(
        span<const Bar> bars, size_t window) const;

    // Rolling annualised Sharpe ratio of daily returns over a window.
    [[nodiscard]] vector<SeriesPoint> rolling_sharpe(
        span<const Bar> bars, size_t window) const;

private:
    // ISO week label "YYYY-Www" derived from a YYYY-MM-DD date string.
    [[nodiscard]] static string iso_week_label(const string& date);
};

} // namespace alphaforge
