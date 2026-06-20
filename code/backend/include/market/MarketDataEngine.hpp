#pragma once

#include "common/Types.hpp"
#include "market/Bar.hpp"
#include "market/IMarketDataObserver.hpp"
#include "market/IMarketDataRepository.hpp"
#include "utils/CsvParser.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

namespace alphaforge {

// Concrete market data engine. Implements the repository read interface and
// acts as the subject in the observer relationship. A shared_mutex gives many
// concurrent readers (risk and analytics workers) while loads take an
// exclusive lock. A small cache of close series avoids recomputing the
// extracted price vectors on hot paths.
class MarketDataEngine final : public IMarketDataRepository {
public:
    MarketDataEngine() = default;

    // Load one symbol from a CSV file. Returns the parse report. Overwrites any
    // existing data for that symbol.
    ParseReport load_symbol(const Symbol& symbol, const filesystem::path& file);

    // Load every *.csv file in a directory, using the file stem as the symbol.
    unordered_map<Symbol, ParseReport> load_directory(
        const filesystem::path& dir);

    // Repository interface.
    [[nodiscard]] bool has_symbol(const Symbol& symbol) const override;
    [[nodiscard]] span<const Bar> history(const Symbol& symbol) const override;
    [[nodiscard]] vector<Price> close_series(const Symbol& symbol) const override;
    [[nodiscard]] optional<Bar> latest_bar(const Symbol& symbol) const override;
    [[nodiscard]] optional<Bar> bar_on(const Symbol& symbol,
                                            const string& date) const override;
    [[nodiscard]] vector<Symbol> symbols() const override;

    [[nodiscard]] size_t bar_count(const Symbol& symbol) const;

    // Observer registration. Observers are held by weak_ptr so the engine never
    // keeps a subscriber alive; expired observers are pruned on publish.
    void subscribe(weak_ptr<IMarketDataObserver> observer);

    // Replay every stored bar for a symbol in chronological order, notifying
    // observers for each. Used to simulate a streaming feed from history.
    void replay(const Symbol& symbol);

private:
    void notify(const Symbol& symbol, const Bar& bar);

    mutable shared_mutex mutex_;
    unordered_map<Symbol, vector<Bar>>   data_;
    mutable unordered_map<Symbol, vector<Price>> close_cache_;
    vector<weak_ptr<IMarketDataObserver>> observers_;

    static const vector<Bar> kEmpty;
};

} // namespace alphaforge
