#pragma once

#include "common/Types.hpp"

using namespace std;

namespace alphaforge {

// A single instrument position using average cost accounting. Long positions
// have positive quantity, short positions negative. The class records realized
// PnL as the position is reduced or flipped and exposes unrealized PnL given a
// current mark price.
class Position {
public:
    Position() = default;
    explicit Position(Symbol symbol) : symbol_(move(symbol)) {}

    [[nodiscard]] const Symbol& symbol() const noexcept { return symbol_; }
    [[nodiscard]] Quantity quantity() const noexcept { return quantity_; }
    [[nodiscard]] Price avg_price() const noexcept { return avg_price_; }
    [[nodiscard]] Money realized_pnl() const noexcept { return realized_pnl_; }
    [[nodiscard]] bool is_flat() const noexcept { return quantity_ == 0; }

    // Apply a fill of signed quantity (positive buy, negative sell) at a price.
    // Updates average cost when increasing exposure and books realized PnL when
    // reducing or flipping. Returns the cash delta (negative when buying).
    Money apply_fill(Quantity signed_qty, Price price);

    [[nodiscard]] Money market_value(Price mark) const noexcept {
        return quantity_ * mark;
    }

    [[nodiscard]] Money unrealized_pnl(Price mark) const noexcept {
        return (mark - avg_price_) * quantity_;
    }

private:
    Symbol   symbol_;
    Quantity quantity_{0};
    Price    avg_price_{0};
    Money    realized_pnl_{0};
};

} // namespace alphaforge
