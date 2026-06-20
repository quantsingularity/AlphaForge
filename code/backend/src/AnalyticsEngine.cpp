#include "analytics/AnalyticsEngine.hpp"

#include "core/Config.hpp"
#include "utils/Statistics.hpp"

#include <charconv>
#include <chrono>
#include <cmath>
#include <format>
#include <span>
#include <string>
#include <vector>

using namespace std;

namespace alphaforge {

namespace {

// Parse a YYYY-MM-DD date. Returns false on any malformed component.
bool parse_iso_date(const string& s, int& y, unsigned& m, unsigned& d) {
    if (s.size() < 10) {
        return false;
    }
    auto to_int = [](const char* a, const char* b, auto& out) {
        return from_chars(a, b, out).ec == errc{};
    };
    return to_int(s.data(), s.data() + 4, y) &&
           to_int(s.data() + 5, s.data() + 7, m) &&
           to_int(s.data() + 8, s.data() + 10, d);
}

// Build close prices from a bar span.
vector<double> closes_of(span<const Bar> bars) {
    vector<double> c;
    c.reserve(bars.size());
    for (const auto& b : bars) {
        c.push_back(b.close);
    }
    return c;
}

// Period returns from the last close of each calendar group (preserving the
// order groups first appear). label_fn maps a date string to a group label.
template <typename LabelFn>
vector<SeriesPoint> grouped_returns(span<const Bar> bars, LabelFn label_fn) {
    vector<string> labels;
    vector<double>      group_close;
    string              current;

    for (const auto& bar : bars) {
        string lbl = label_fn(bar.date);
        if (lbl != current) {
            labels.push_back(lbl);
            group_close.push_back(bar.close);
            current = lbl;
        } else {
            group_close.back() = bar.close;  // last close wins within the group
        }
    }

    vector<SeriesPoint> out;
    for (size_t i = 1; i < group_close.size(); ++i) {
        const double prev = group_close[i - 1];
        const double ret  = prev != 0.0 ? group_close[i] / prev - 1.0 : 0.0;
        out.push_back(SeriesPoint{labels[i], ret});
    }
    return out;
}

} // namespace

string AnalyticsEngine::iso_week_label(const string& date) {
    int y{};
    unsigned m{}, d{};
    if (!parse_iso_date(date, y, m, d)) {
        return date.substr(0, min<size_t>(date.size(), 7));
    }
    using namespace chrono;
    const year_month_day ymd{year{y}, month{m}, day{d}};
    if (!ymd.ok()) {
        return date.substr(0, 7);
    }
    const sys_days sd = ymd;
    const unsigned wd = weekday{sd}.iso_encoding();        // 1=Mon .. 7=Sun
    const sys_days thursday = sd + days{4 - static_cast<int>(wd)};
    const year_month_day thu_ymd{thursday};
    const int iso_year = static_cast<int>(thu_ymd.year());
    const sys_days jan1 = year_month_day{thu_ymd.year(), January, day{1}};
    const int week = static_cast<int>((thursday - jan1).count() / 7) + 1;
    return format("{:04d}-W{:02d}", iso_year, week);
}

vector<double> AnalyticsEngine::daily_returns(span<const double> closes) const {
    return stats::simple_returns(closes);
}

vector<SeriesPoint> AnalyticsEngine::weekly_returns(span<const Bar> bars) const {
    return grouped_returns(bars, [](const string& d) { return iso_week_label(d); });
}

vector<SeriesPoint> AnalyticsEngine::monthly_returns(span<const Bar> bars) const {
    return grouped_returns(bars, [](const string& d) {
        return d.substr(0, min<size_t>(d.size(), 7));
    });
}

double AnalyticsEngine::annualized_return(span<const double> closes) const {
    if (closes.size() < 2 || closes.front() <= 0.0) {
        return 0.0;
    }
    const double periods = Config::instance().periods_per_year();
    const double total_growth = closes.back() / closes.front();
    const double years = static_cast<double>(closes.size() - 1) / periods;
    if (years <= 0.0 || total_growth <= 0.0) {
        return 0.0;
    }
    return pow(total_growth, 1.0 / years) - 1.0;
}

vector<SeriesPoint> AnalyticsEngine::rolling_volatility(
    span<const Bar> bars, size_t window) const {
    vector<SeriesPoint> out;
    if (window < 2 || bars.size() <= window) {
        return out;
    }
    const auto closes = closes_of(bars);
    const auto rets   = stats::simple_returns(span<const double>{closes});
    const double periods = Config::instance().periods_per_year();

    for (size_t end = window; end <= rets.size(); ++end) {
        span<const double> win{rets.data() + (end - window), window};
        const auto sd = stats::stdev(win);
        const double vol = sd ? *sd * sqrt(periods) : 0.0;
        // rets[i] corresponds to bars[i+1]; window end index maps to bar end.
        out.push_back(SeriesPoint{bars[end].date, vol});
    }
    return out;
}

vector<SeriesPoint> AnalyticsEngine::rolling_sharpe(
    span<const Bar> bars, size_t window) const {
    vector<SeriesPoint> out;
    if (window < 2 || bars.size() <= window) {
        return out;
    }
    const auto closes = closes_of(bars);
    const auto rets   = stats::simple_returns(span<const double>{closes});
    const double periods = Config::instance().periods_per_year();
    const double rf_per  = Config::instance().risk_free_rate() / periods;

    for (size_t end = window; end <= rets.size(); ++end) {
        span<const double> win{rets.data() + (end - window), window};
        const auto m  = stats::mean(win);
        const auto sd = stats::stdev(win);
        double sharpe = 0.0;
        if (m && sd && *sd != 0.0) {
            sharpe = ((*m - rf_per) / *sd) * sqrt(periods);
        }
        out.push_back(SeriesPoint{bars[end].date, sharpe});
    }
    return out;
}

} // namespace alphaforge
