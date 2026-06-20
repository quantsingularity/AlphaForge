#pragma once

#include "common/Types.hpp"

#include <compare>
#include <string>

using namespace std;

namespace alphaforge {

// A single OHLCV bar. Date is kept as an ISO 8601 string (YYYY-MM-DD); ISO
// dates sort lexicographically in chronological order, which keeps the data
// layer free of a calendar dependency. valid() enforces the basic OHLC
// invariants that any sane bar must satisfy.
struct Bar {
    string   date;
    Price         open{0};
    Price         high{0};
    Price         low{0};
    Price         close{0};
    long long     volume{0};

    [[nodiscard]] bool valid() const noexcept {
        if (date.empty()) {
            return false;
        }
        if (open <= 0 || high <= 0 || low <= 0 || close <= 0) {
            return false;
        }
        if (low > high) {
            return false;
        }
        if (open > high || open < low) {
            return false;
        }
        if (close > high || close < low) {
            return false;
        }
        if (volume < 0) {
            return false;
        }
        return true;
    }

    // Order bars by date so a vector of bars is a time series.
    [[nodiscard]] friend bool operator<(const Bar& a, const Bar& b) noexcept {
        return a.date < b.date;
    }
};

} // namespace alphaforge
