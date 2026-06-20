#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <stdexcept>

using namespace std;

namespace alphaforge {

// Fundamental domain aliases. Kept as strong-ish aliases so call sites read
// like finance code rather than a sea of doubles and strings.
using Symbol   = string;
using Price    = double;
using Quantity = double;   // fractional shares supported
using Money    = double;   // cash amounts in account currency

// Side of an order or a position.
enum class Side {
    Buy,
    Sell
};

// Order types supported by the execution layer.
enum class OrderType {
    Market,
    Limit,
    Stop
};

// Lifecycle states for an order.
enum class OrderStatus {
    Pending,
    Submitted,
    Filled,
    Cancelled,
    Rejected
};

// Strategy identifiers used by the factory.
enum class StrategyType {
    BuyAndHold,
    MovingAverageCrossover,
    Momentum,
    MeanReversion
};

// Enum to-string converters are named as_string so they do not hide the
// standard numeric to_string brought in by using namespace std.
[[nodiscard]] inline string_view as_string(Side s) noexcept {
    switch (s) {
        case Side::Buy:  return "BUY";
        case Side::Sell: return "SELL";
    }
    return "UNKNOWN";
}

[[nodiscard]] inline string_view as_string(OrderType t) noexcept {
    switch (t) {
        case OrderType::Market: return "MARKET";
        case OrderType::Limit:  return "LIMIT";
        case OrderType::Stop:   return "STOP";
    }
    return "UNKNOWN";
}

[[nodiscard]] inline string_view as_string(OrderStatus s) noexcept {
    switch (s) {
        case OrderStatus::Pending:   return "PENDING";
        case OrderStatus::Submitted: return "SUBMITTED";
        case OrderStatus::Filled:    return "FILLED";
        case OrderStatus::Cancelled: return "CANCELLED";
        case OrderStatus::Rejected:  return "REJECTED";
    }
    return "UNKNOWN";
}

[[nodiscard]] inline string_view as_string(StrategyType t) noexcept {
    switch (t) {
        case StrategyType::BuyAndHold:             return "BuyAndHold";
        case StrategyType::MovingAverageCrossover: return "MovingAverageCrossover";
        case StrategyType::Momentum:               return "Momentum";
        case StrategyType::MeanReversion:          return "MeanReversion";
    }
    return "Unknown";
}

// Exception type used across the platform for domain level failures.
class AlphaForgeError : public runtime_error {
public:
    using runtime_error::runtime_error;
};

} // namespace alphaforge
