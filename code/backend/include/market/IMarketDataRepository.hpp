#pragma once

#include "common/Types.hpp"
#include "market/Bar.hpp"

#include <optional>
#include <span>
#include <vector>

using namespace std;

namespace alphaforge {

// Repository pattern: the read side of market data access decoupled from how
// the data is sourced (CSV today, a database or feed tomorrow). The strategy,
// risk and analytics layers depend on this interface, not on the concrete CSV
// backed engine.
class IMarketDataRepository {
public:
    virtual ~IMarketDataRepository() = default;

    [[nodiscard]] virtual bool has_symbol(const Symbol& symbol) const = 0;

    // Full history for a symbol as a non owning view. Empty if unknown.
    [[nodiscard]] virtual span<const Bar> history(const Symbol& symbol) const = 0;

    // Closing prices extracted into a fresh vector for numeric routines.
    [[nodiscard]] virtual vector<Price> close_series(const Symbol& symbol) const = 0;

    // Most recent bar, or nullopt if the symbol has no data.
    [[nodiscard]] virtual optional<Bar> latest_bar(const Symbol& symbol) const = 0;

    // Bar on a specific date, or nullopt.
    [[nodiscard]] virtual optional<Bar> bar_on(const Symbol& symbol,
                                                    const string& date) const = 0;

    [[nodiscard]] virtual vector<Symbol> symbols() const = 0;
};

} // namespace alphaforge
