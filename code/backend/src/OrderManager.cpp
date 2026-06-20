#include "execution/OrderManager.hpp"

#include <algorithm>

using namespace std;

namespace alphaforge {

uint64_t OrderManager::submit(const Symbol& symbol, Side side, OrderType type,
                                   Quantity quantity,
                                   optional<Price> limit_price,
                                   optional<Price> stop_price) {
    const uint64_t id = next_id_++;
    Order order(id, symbol, side, type, quantity, limit_price, stop_price);
    order.set_status(OrderStatus::Submitted);
    pending_.push(order);
    all_.push_back(order);
    return id;
}

bool OrderManager::is_marketable(const Order& order, Price ref) const {
    switch (order.type()) {
        case OrderType::Market:
            return true;
        case OrderType::Limit:
            if (!order.limit_price()) {
                return false;
            }
            return order.side() == Side::Buy ? ref <= *order.limit_price()
                                             : ref >= *order.limit_price();
        case OrderType::Stop:
            if (!order.stop_price()) {
                return false;
            }
            return order.side() == Side::Buy ? ref >= *order.stop_price()
                                             : ref <= *order.stop_price();
    }
    return false;
}

optional<ExecutionResult> OrderManager::process_next(Price reference_price) {
    if (pending_.empty()) {
        return nullopt;
    }
    Order order = pending_.front();
    pending_.pop();

    ExecutionResult result;
    result.order_id = order.id();

    if (!is_marketable(order, reference_price)) {
        order.set_status(OrderStatus::Rejected);
        result.status = OrderStatus::Rejected;
        result.reason = "not marketable at reference price";
    } else {
        order.set_status(OrderStatus::Filled);
        order.set_fill_price(reference_price);
        portfolio_.execute(order.symbol(), order.side(), order.quantity(),
                           reference_price);
        history_.push(order);
        result.status     = OrderStatus::Filled;
        result.fill_price = reference_price;
    }

    // Reflect the terminal status in the audit trail.
    auto it = ranges::find_if(
        all_, [&](const Order& o) { return o.id() == order.id(); });
    if (it != all_.end()) {
        *it = order;
    }
    return result;
}

vector<ExecutionResult> OrderManager::process_all(Price reference_price) {
    vector<ExecutionResult> results;
    while (auto r = process_next(reference_price)) {
        results.push_back(*r);
    }
    return results;
}

bool OrderManager::undo_last_trade() {
    if (history_.empty()) {
        return false;
    }
    Order last = history_.top();
    history_.pop();

    const bool ok = portfolio_.undo_last_fill();

    last.set_status(OrderStatus::Cancelled);
    auto it = ranges::find_if(
        all_, [&](const Order& o) { return o.id() == last.id(); });
    if (it != all_.end()) {
        *it = last;
    }
    return ok;
}

} // namespace alphaforge
