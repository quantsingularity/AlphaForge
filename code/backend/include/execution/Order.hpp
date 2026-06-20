#pragma once

#include "common/Types.hpp"

#include <optional>
#include <string>

using namespace std;

namespace alphaforge {

// An order flowing through the execution system. Limit orders carry a limit
// price, stop orders a stop price; market orders leave both empty. The status
// advances through its lifecycle as the order manager processes it.
class Order {
public:
    Order(uint64_t id, Symbol symbol, Side side, OrderType type,
          Quantity quantity, optional<Price> limit_price = nullopt,
          optional<Price> stop_price = nullopt)
        : id_(id),
          symbol_(move(symbol)),
          side_(side),
          type_(type),
          quantity_(quantity),
          limit_price_(limit_price),
          stop_price_(stop_price) {}

    [[nodiscard]] uint64_t id() const noexcept { return id_; }
    [[nodiscard]] const Symbol& symbol() const noexcept { return symbol_; }
    [[nodiscard]] Side side() const noexcept { return side_; }
    [[nodiscard]] OrderType type() const noexcept { return type_; }
    [[nodiscard]] Quantity quantity() const noexcept { return quantity_; }
    [[nodiscard]] optional<Price> limit_price() const noexcept { return limit_price_; }
    [[nodiscard]] optional<Price> stop_price() const noexcept { return stop_price_; }

    [[nodiscard]] OrderStatus status() const noexcept { return status_; }
    void set_status(OrderStatus s) noexcept { status_ = s; }

    [[nodiscard]] optional<Price> fill_price() const noexcept { return fill_price_; }
    void set_fill_price(Price p) noexcept { fill_price_ = p; }

private:
    uint64_t        id_;
    Symbol               symbol_;
    Side                 side_;
    OrderType            type_;
    Quantity             quantity_;
    optional<Price> limit_price_;
    optional<Price> stop_price_;
    OrderStatus          status_{OrderStatus::Pending};
    optional<Price> fill_price_;
};

} // namespace alphaforge
