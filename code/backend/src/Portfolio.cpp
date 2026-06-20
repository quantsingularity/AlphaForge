#include "portfolio/Portfolio.hpp"

#include <cmath>
#include <utility>

using namespace std;

namespace alphaforge {

Portfolio::Portfolio(Money initial_cash)
    : initial_cash_(initial_cash), cash_(initial_cash) {}

void Portfolio::execute(const Symbol& symbol, Side side, Quantity quantity,
                        Price price, const string& timestamp) {
    const Quantity signed_qty = (side == Side::Buy) ? quantity : -quantity;

    auto [it, inserted] = positions_.try_emplace(symbol, Position{symbol});
    const Money cash_delta = it->second.apply_fill(signed_qty, price);
    cash_ += cash_delta;

    marks_[symbol] = price;
    blotter_.push_back(Fill{symbol, side, quantity, price, cash_delta, timestamp});
}

bool Portfolio::undo_last_fill() {
    if (blotter_.empty()) {
        return false;
    }
    blotter_.pop_back();

    // Rebuild deterministic state by replaying the remaining blotter from the
    // initial cash. This avoids fragile inverse accounting for average cost
    // positions and is always correct by construction.
    auto remaining = move(blotter_);
    blotter_.clear();
    cash_ = initial_cash_;
    positions_.clear();
    for (const auto& fill : remaining) {
        execute(fill.symbol, fill.side, fill.quantity, fill.price, fill.timestamp);
    }
    return true;
}

void Portfolio::on_bar(const Symbol& symbol, const Bar& bar) {
    marks_[symbol] = bar.close;
}

void Portfolio::set_mark(const Symbol& symbol, Price price) {
    marks_[symbol] = price;
}

optional<Price> Portfolio::mark(const Symbol& symbol) const {
    const auto it = marks_.find(symbol);
    return it == marks_.end() ? nullopt : optional<Price>{it->second};
}

optional<Position> Portfolio::position(const Symbol& symbol) const {
    const auto it = positions_.find(symbol);
    return it == positions_.end() ? nullopt : optional<Position>{it->second};
}

vector<Position> Portfolio::positions() const {
    vector<Position> out;
    out.reserve(positions_.size());
    for (const auto& [symbol, pos] : positions_) {
        if (!pos.is_flat()) {
            out.push_back(pos);
        }
    }
    return out;
}

Money Portfolio::holdings_value() const {
    Money value = 0;
    for (const auto& [symbol, pos] : positions_) {
        if (pos.is_flat()) {
            continue;
        }
        const auto m = marks_.find(symbol);
        const Price price = (m != marks_.end()) ? m->second : pos.avg_price();
        value += pos.market_value(price);
    }
    return value;
}

Money Portfolio::total_value() const {
    return cash_ + holdings_value();
}

Money Portfolio::unrealized_pnl() const {
    Money pnl = 0;
    for (const auto& [symbol, pos] : positions_) {
        if (pos.is_flat()) {
            continue;
        }
        const auto m = marks_.find(symbol);
        const Price price = (m != marks_.end()) ? m->second : pos.avg_price();
        pnl += pos.unrealized_pnl(price);
    }
    return pnl;
}

Money Portfolio::realized_pnl() const {
    Money pnl = 0;
    for (const auto& [symbol, pos] : positions_) {
        pnl += pos.realized_pnl();
    }
    return pnl;
}

Money Portfolio::gross_exposure() const {
    Money gross = 0;
    for (const auto& [symbol, pos] : positions_) {
        if (pos.is_flat()) {
            continue;
        }
        const auto m = marks_.find(symbol);
        const Price price = (m != marks_.end()) ? m->second : pos.avg_price();
        gross += abs(pos.market_value(price));
    }
    return gross;
}

Money Portfolio::net_exposure() const {
    Money net = 0;
    for (const auto& [symbol, pos] : positions_) {
        if (pos.is_flat()) {
            continue;
        }
        const auto m = marks_.find(symbol);
        const Price price = (m != marks_.end()) ? m->second : pos.avg_price();
        net += pos.market_value(price);
    }
    return net;
}

vector<PositionWeight> Portfolio::weights() const {
    vector<PositionWeight> out;
    const Money total = total_value();
    if (total == 0) {
        return out;
    }
    for (const auto& [symbol, pos] : positions_) {
        if (pos.is_flat()) {
            continue;
        }
        const auto m = marks_.find(symbol);
        const Price price = (m != marks_.end()) ? m->second : pos.avg_price();
        out.push_back(PositionWeight{symbol, pos.market_value(price) / total});
    }
    return out;
}

} // namespace alphaforge
