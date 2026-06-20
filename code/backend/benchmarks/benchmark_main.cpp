// AlphaForge micro benchmarks.
//
// Times the hot paths of the engine: data loading, return computation, strategy
// evaluation, a full backtest and the parallel risk batch. Numbers are wall
// clock milliseconds on this machine and are meant for relative comparison, not
// as absolute throughput claims.

#include "backtesting/Backtester.hpp"
#include "backtesting/StrategyFactory.hpp"
#include "core/Application.hpp"
#include "core/Config.hpp"
#include "risk/RiskEngine.hpp"
#include "utils/Statistics.hpp"
#include "utils/Timer.hpp"

#include <iomanip>
#include <iostream>
#include <span>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace std;

using namespace alphaforge;

namespace {
void row(const string& label, double ms, const string& note = "") {
    cout << left << setw(40) << label << right << setw(12)
              << fixed << setprecision(3) << ms << " ms   " << note << "\n";
}
}  // namespace

int main(int argc, char** argv) {
    const string data_dir = (argc > 1) ? argv[1] : "data";
    Config::instance().set_data_directory(data_dir);

    Application app;
    const double load_ms = time_call([&] { app.bootstrap(); });

    const auto symbols = app.market().symbols();
    if (symbols.empty()) {
        cout << "No data found in '" << data_dir << "'. Generate it first.\n";
        return 1;
    }
    const auto& sym = symbols.front();

    cout << string(64, '=') << "\n  AlphaForge Benchmark Report\n"
              << string(64, '=') << "\n";
    cout << "Symbols: " << symbols.size() << ", threads: " << app.pool().size()
              << "\n\n";

    row("Load market data (" + to_string(symbols.size()) + " symbols)", load_ms);

    const double closes_ms = time_call([&] {
        for (int i = 0; i < 100; ++i) {
            auto c = app.market().close_series(sym);
            (void)c;
        }
    });
    row("Close series x100 (" + sym + ")", closes_ms);

    auto closes = app.market().close_series(sym);
    const double ret_ms = time_call([&] {
        for (int i = 0; i < 100; ++i) {
            auto r = stats::simple_returns(span<const Price>{closes});
            (void)r;
        }
    });
    row("Simple returns x100", ret_ms);

    auto strat = StrategyFactory::create(StrategyType::MovingAverageCrossover);
    Backtester bt;
    const double bt_ms = time_call([&] {
        for (int i = 0; i < 50; ++i) {
            auto r = bt.run(*strat, app.market().history(sym), sym);
            (void)r;
        }
    });
    row("Backtest x50 (MA crossover)", bt_ms);

    unordered_map<Symbol, vector<double>> rets;
    for (const auto& s : symbols) {
        auto c = app.market().close_series(s);
        rets[s] = stats::simple_returns(span<const Price>{c});
    }
    const double seq_ms = time_call([&] {
        for (const auto& [s, r] : rets) {
            auto m = app.risk().compute(span<const double>{r});
            (void)m;
        }
    });
    row("Risk metrics sequential", seq_ms);

    const double par_ms =
        time_call([&] { auto b = app.risk().compute_batch(rets, app.pool()); (void)b; });
    row("Risk metrics parallel batch", par_ms,
        app.pool().size() > 1 ? "" : "(single core machine: expect no speedup)");

    cout << "\nNote: this sandbox reports "
              << thread::hardware_concurrency()
              << " hardware thread(s); parallel timings reflect that.\n";
    return 0;
}
