#include "market/MarketDataEngine.hpp"

#include "utils/Logger.hpp"

#include <algorithm>

using namespace std;

namespace alphaforge {

const vector<Bar> MarketDataEngine::kEmpty{};

ParseReport MarketDataEngine::load_symbol(const Symbol& symbol,
                                          const filesystem::path& file) {
    ParseReport report;
    auto bars = CsvParser::parse_file(file, report);

    unique_lock lock(mutex_);
    data_[symbol] = move(bars);
    close_cache_.erase(symbol);
    return report;
}

unordered_map<Symbol, ParseReport> MarketDataEngine::load_directory(
    const filesystem::path& dir) {
    unordered_map<Symbol, ParseReport> reports;
    if (!filesystem::exists(dir) || !filesystem::is_directory(dir)) {
        Logger::instance().warn("data directory not found: " + dir.string());
        return reports;
    }
    for (const auto& entry : filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        if (entry.path().extension() != ".csv") {
            continue;
        }
        const Symbol symbol = entry.path().stem().string();
        reports[symbol] = load_symbol(symbol, entry.path());
    }
    return reports;
}

bool MarketDataEngine::has_symbol(const Symbol& symbol) const {
    shared_lock lock(mutex_);
    return data_.contains(symbol);
}

span<const Bar> MarketDataEngine::history(const Symbol& symbol) const {
    shared_lock lock(mutex_);
    const auto it = data_.find(symbol);
    if (it == data_.end()) {
        return span<const Bar>{kEmpty};
    }
    // The vector is stable for the lifetime of the engine after load; returning
    // a span over it is safe for the read mostly access pattern used here.
    return span<const Bar>{it->second};
}

vector<Price> MarketDataEngine::close_series(const Symbol& symbol) const {
    {
        shared_lock lock(mutex_);
        const auto cached = close_cache_.find(symbol);
        if (cached != close_cache_.end()) {
            return cached->second;
        }
    }
    unique_lock lock(mutex_);
    const auto it = data_.find(symbol);
    if (it == data_.end()) {
        return {};
    }
    vector<Price> closes;
    closes.reserve(it->second.size());
    for (const auto& bar : it->second) {
        closes.push_back(bar.close);
    }
    close_cache_[symbol] = closes;
    return closes;
}

optional<Bar> MarketDataEngine::latest_bar(const Symbol& symbol) const {
    shared_lock lock(mutex_);
    const auto it = data_.find(symbol);
    if (it == data_.end() || it->second.empty()) {
        return nullopt;
    }
    return it->second.back();
}

optional<Bar> MarketDataEngine::bar_on(const Symbol& symbol,
                                            const string& date) const {
    shared_lock lock(mutex_);
    const auto it = data_.find(symbol);
    if (it == data_.end()) {
        return nullopt;
    }
    const auto found = ranges::lower_bound(it->second, date, {}, &Bar::date);
    if (found != it->second.end() && found->date == date) {
        return *found;
    }
    return nullopt;
}

vector<Symbol> MarketDataEngine::symbols() const {
    shared_lock lock(mutex_);
    vector<Symbol> out;
    out.reserve(data_.size());
    for (const auto& [symbol, bars] : data_) {
        out.push_back(symbol);
    }
    ranges::sort(out);
    return out;
}

size_t MarketDataEngine::bar_count(const Symbol& symbol) const {
    shared_lock lock(mutex_);
    const auto it = data_.find(symbol);
    return it == data_.end() ? 0 : it->second.size();
}

void MarketDataEngine::subscribe(weak_ptr<IMarketDataObserver> observer) {
    unique_lock lock(mutex_);
    observers_.push_back(move(observer));
}

void MarketDataEngine::notify(const Symbol& symbol, const Bar& bar) {
    vector<shared_ptr<IMarketDataObserver>> live;
    {
        unique_lock lock(mutex_);
        erase_if(observers_, [](const weak_ptr<IMarketDataObserver>& w) {
            return w.expired();
        });
        live.reserve(observers_.size());
        for (auto& w : observers_) {
            if (auto sp = w.lock()) {
                live.push_back(move(sp));
            }
        }
    }
    // Notify outside the lock so observers can call back into the engine.
    for (auto& obs : live) {
        obs->on_bar(symbol, bar);
    }
}

void MarketDataEngine::replay(const Symbol& symbol) {
    vector<Bar> snapshot;
    {
        shared_lock lock(mutex_);
        const auto it = data_.find(symbol);
        if (it == data_.end()) {
            return;
        }
        snapshot = it->second;
    }
    for (const auto& bar : snapshot) {
        notify(symbol, bar);
    }
}

} // namespace alphaforge
