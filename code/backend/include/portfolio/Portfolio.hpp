#pragma once

#include "common/Types.hpp"
#include "market/IMarketDataObserver.hpp"
#include "portfolio/Position.hpp"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

namespace alphaforge {

// A fill record produced when an order executes against the portfolio.
struct Fill {
    Symbol   symbol;
    Side     side{Side::Buy};
    Quantity quantity{0};
    Price    price{0};
    Money    cash_delta{0};   // signed effect on cash including the fill
    string timestamp;    // ISO date or wall clock, informational
};

// Aggregated weight of a single position in the portfolio.
struct PositionWeight {
    Symbol symbol;
    double weight{0};   // market value divided by total portfolio value
};

// The portfolio holds cash and a set of positions. It marks positions to the
// latest observed prices (it is a market data observer) and reports valuation,
// exposure and PnL. It is the single source of truth the order manager mutates.
class Portfolio final : public IMarketDataObserver {
public:
    explicit Portfolio(Money initial_cash);

    [[nodiscard]] Money cash() const noexcept { return cash_; }
    [[nodiscard]] Money initial_cash() const noexcept { return initial_cash_; }

    // Execute a fill: adjusts cash and the relevant position, books realized
    // PnL inside the position. Records the fill in the blotter.
    void execute(const Symbol& symbol, Side side, Quantity quantity, Price price,
                 const string& timestamp = "");

    // Reverse the most recent fill. Used by the order manager undo feature.
    // Returns false if there is nothing to undo.
    bool undo_last_fill();

    // Observer hook: store the latest price so valuation uses current marks.
    void on_bar(const Symbol& symbol, const Bar& bar) override;

    void set_mark(const Symbol& symbol, Price price);
    [[nodiscard]] optional<Price> mark(const Symbol& symbol) const;

    [[nodiscard]] optional<Position> position(const Symbol& symbol) const;
    [[nodiscard]] vector<Position> positions() const;

    // Valuation and exposure.
    [[nodiscard]] Money holdings_value() const;       // sum of position market values
    [[nodiscard]] Money total_value() const;          // cash + holdings
    [[nodiscard]] Money unrealized_pnl() const;
    [[nodiscard]] Money realized_pnl() const;
    [[nodiscard]] Money gross_exposure() const;       // sum of absolute market values
    [[nodiscard]] Money net_exposure() const;         // sum of signed market values
    [[nodiscard]] vector<PositionWeight> weights() const;

    [[nodiscard]] const vector<Fill>& blotter() const noexcept { return blotter_; }

private:
    Money initial_cash_;
    Money cash_;
    unordered_map<Symbol, Position> positions_;
    unordered_map<Symbol, Price>    marks_;
    vector<Fill>                    blotter_;
};

} // namespace alphaforge
