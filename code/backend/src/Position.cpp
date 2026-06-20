#include "portfolio/Position.hpp"

#include <cmath>
#include <algorithm>

using namespace std;

namespace alphaforge {

Money Position::apply_fill(Quantity signed_qty, Price price) {
    const Money cash_delta = -signed_qty * price;

    const bool opening_or_adding =
        quantity_ == 0 || ((quantity_ > 0) == (signed_qty > 0));

    if (opening_or_adding) {
        const Quantity new_qty = quantity_ + signed_qty;
        if (new_qty != 0) {
            avg_price_ = (quantity_ * avg_price_ + signed_qty * price) / new_qty;
        }
        quantity_ = new_qty;
    } else {
        // Opposite direction: book realized PnL on the closed portion.
        const Quantity closing = min(abs(signed_qty), abs(quantity_));
        const double   side    = quantity_ > 0 ? 1.0 : -1.0;
        realized_pnl_ += closing * (price - avg_price_) * side;

        const Quantity new_qty = quantity_ + signed_qty;
        if (new_qty == 0) {
            avg_price_ = 0;
        } else if ((quantity_ > 0) != (new_qty > 0)) {
            // Position flipped sides; the remainder opens at the fill price.
            avg_price_ = price;
        }
        // Pure reduction on the same side leaves the average cost unchanged.
        quantity_ = new_qty;
    }

    return cash_delta;
}

} // namespace alphaforge
