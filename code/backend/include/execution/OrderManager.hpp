#pragma once

#include "common/Types.hpp"
#include "execution/Order.hpp"
#include "portfolio/Portfolio.hpp"

#include <cstdint>
#include <optional>
#include <queue>
#include <stack>
#include <vector>

using namespace std;

namespace alphaforge {

// Result of attempting to execute a single order.
struct ExecutionResult {
    uint64_t order_id{0};
    OrderStatus   status{OrderStatus::Rejected};
    optional<Price> fill_price;
    string reason;   // populated on rejection
};

// The order management system. Newly created orders enter a FIFO queue. On
// process() the front order is matched against a reference price using a simple
// marketable fill model (market orders always fill, limit and stop orders fill
// only when the reference price satisfies their condition). Executed orders are
// pushed onto a history stack which powers undo_last_trade().
//
// The OMS does not own the portfolio; it holds a reference and mutates it on
// fills, keeping a single source of truth for cash and positions.
class OrderManager {
public:
    explicit OrderManager(Portfolio& portfolio) : portfolio_(portfolio) {}

    // Create and enqueue an order. Returns the assigned order id.
    uint64_t submit(const Symbol& symbol, Side side, OrderType type,
                         Quantity quantity,
                         optional<Price> limit_price = nullopt,
                         optional<Price> stop_price = nullopt);

    // Process the next queued order against a reference price (typically the
    // latest close). Returns nullopt if the queue is empty.
    optional<ExecutionResult> process_next(Price reference_price);

    // Drain the whole queue against a single reference price.
    vector<ExecutionResult> process_all(Price reference_price);

    // Reverse the most recently executed trade, restoring the portfolio.
    // Returns false if there is no executed trade to undo.
    bool undo_last_trade();

    [[nodiscard]] size_t pending_count() const noexcept { return pending_.size(); }
    [[nodiscard]] size_t executed_count() const noexcept { return history_.size(); }
    [[nodiscard]] const vector<Order>& all_orders() const noexcept { return all_; }

private:
    [[nodiscard]] bool is_marketable(const Order& order, Price ref) const;

    Portfolio&             portfolio_;
    queue<Order>      pending_;
    stack<Order>      history_;   // executed orders, most recent on top
    vector<Order>     all_;       // audit trail of every order seen
    uint64_t          next_id_{1};
};

} // namespace alphaforge
