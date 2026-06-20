// AlphaForge interactive CLI dashboard.
//
// A text menu over the same Application facade the REST server uses. Useful for
// quick exploration and for environments without the web frontend.

#include "core/Application.hpp"
#include "backtesting/Backtester.hpp"
#include "backtesting/StrategyFactory.hpp"
#include "utils/Logger.hpp"
#include "utils/Statistics.hpp"
#include "utils/Timer.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <limits>
#include <set>
#include <span>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

using namespace alphaforge;

namespace {

string read_line(const string& prompt) {
    cout << prompt;
    string s;
    getline(cin, s);
    return s;
}

void print_header(const string& title) {
    cout << "\n" << string(64, '=') << "\n  " << title << "\n"
              << string(64, '=') << "\n";
}

void show_menu() {
    print_header("AlphaForge Quant Terminal");
    cout << "  1.  Load Market Data\n"
                 "  2.  View Market Data\n"
                 "  3.  Portfolio Dashboard\n"
                 "  4.  Submit Order\n"
                 "  5.  Run Backtest\n"
                 "  6.  Risk Analysis\n"
                 "  7.  Analytics Dashboard\n"
                 "  8.  Performance Benchmark Report\n"
                 "  9.  Watchlist\n"
                 "  10. Undo Last Trade\n"
                 "  11. Exit\n";
}

void load_data(Application& app) {
    const string dir = read_line("Data directory [data]: ");
    if (!dir.empty()) {
        Config::instance().set_data_directory(dir);
    }
    const auto loaded = app.bootstrap();
    cout << "Loaded " << loaded << " bars across "
              << app.market().symbols().size() << " symbols.\n";
}

void view_market(Application& app) {
    const string symbol = read_line("Symbol: ");
    if (!app.market().has_symbol(symbol)) {
        cout << "Unknown symbol.\n";
        return;
    }
    auto hist = app.market().history(symbol);
    const size_t show = min<size_t>(10, hist.size());
    cout << fixed << setprecision(2);
    cout << "\nLast " << show << " bars for " << symbol << ":\n";
    cout << left << setw(12) << "Date" << right << setw(10)
              << "Open" << setw(10) << "High" << setw(10) << "Low"
              << setw(10) << "Close" << setw(14) << "Volume" << "\n";
    for (size_t i = hist.size() - show; i < hist.size(); ++i) {
        const auto& b = hist[i];
        cout << left << setw(12) << b.date << right
                  << setw(10) << b.open << setw(10) << b.high
                  << setw(10) << b.low << setw(10) << b.close
                  << setw(14) << b.volume << "\n";
    }
}

void portfolio_dashboard(Application& app) {
    auto& pf = app.portfolio();
    cout << fixed << setprecision(2);
    print_header("Portfolio Dashboard");
    cout << "Cash:            " << pf.cash() << "\n";
    cout << "Holdings value:  " << pf.holdings_value() << "\n";
    cout << "Total value:     " << pf.total_value() << "\n";
    cout << "Realized PnL:    " << pf.realized_pnl() << "\n";
    cout << "Unrealized PnL:  " << pf.unrealized_pnl() << "\n";
    cout << "Gross exposure:  " << pf.gross_exposure() << "\n";
    cout << "Net exposure:    " << pf.net_exposure() << "\n";
    const auto positions = pf.positions();
    if (positions.empty()) {
        cout << "\nNo open positions.\n";
        return;
    }
    cout << "\nPositions:\n";
    for (const auto& pos : positions) {
        const auto mark = pf.mark(pos.symbol());
        cout << "  " << left << setw(8) << pos.symbol()
                  << " qty " << right << setw(10) << pos.quantity()
                  << " @ " << setw(10) << pos.avg_price();
        if (mark) {
            cout << "  uPnL " << setw(10) << pos.unrealized_pnl(*mark);
        }
        cout << "\n";
    }
}

void submit_order(Application& app) {
    const string symbol = read_line("Symbol: ");
    if (!app.market().has_symbol(symbol)) {
        cout << "Unknown symbol.\n";
        return;
    }
    const string side_s = read_line("Side (buy/sell): ");
    const string qty_s  = read_line("Quantity: ");
    double quantity = 0;
    try {
        quantity = stod(qty_s);
    } catch (...) {
        cout << "Invalid quantity.\n";
        return;
    }
    auto last = app.market().latest_bar(symbol);
    if (!last) {
        cout << "No price available.\n";
        return;
    }
    const Side side = (side_s == "sell") ? Side::Sell : Side::Buy;
    const auto id = app.orders().submit(symbol, side, OrderType::Market, quantity);
    const auto result = app.orders().process_next(last->close);
    cout << "Order " << id << " ";
    if (result && result->status == OrderStatus::Filled) {
        cout << "filled at " << *result->fill_price << "\n";
    } else {
        cout << "rejected\n";
    }
}

void run_backtest(Application& app) {
    const string symbol = read_line("Symbol: ");
    if (!app.market().has_symbol(symbol)) {
        cout << "Unknown symbol.\n";
        return;
    }
    cout << "Strategies: ";
    for (const auto& s : StrategyFactory::available()) cout << s << " ";
    cout << "\n";
    const string strat_name = read_line("Strategy: ");
    auto strat = StrategyFactory::create(strat_name);
    if (!strat) {
        cout << "Unknown strategy.\n";
        return;
    }
    BacktestConfig cfg;
    cfg.initial_capital      = Config::instance().initial_capital();
    cfg.transaction_cost_bps = Config::instance().transaction_cost_bps();
    cfg.slippage_bps         = Config::instance().slippage_bps();

    Backtester bt(cfg);
    const auto result = bt.run(*strat, app.market().history(symbol), symbol);
    const auto& m = result.metrics;
    cout << fixed << setprecision(4);
    print_header("Backtest: " + result.strategy + " on " + symbol);
    cout << "Total return:    " << m.total_return * 100 << " %\n";
    cout << "CAGR:            " << m.cagr * 100 << " %\n";
    cout << "Volatility:      " << m.annualized_volatility * 100 << " %\n";
    cout << "Sharpe:          " << m.sharpe_ratio << "\n";
    cout << "Sortino:         " << m.sortino_ratio << "\n";
    cout << "Max drawdown:    " << m.max_drawdown * 100 << " %\n";
    cout << "Win rate:        " << m.win_rate * 100 << " %\n";
    cout << "Profit factor:   " << m.profit_factor << "\n";
    cout << "Trades:          " << m.num_trades << "\n";
    cout << "\nBenchmark (buy and hold) CAGR: " << result.benchmark.cagr * 100
              << " %, Sharpe: " << result.benchmark.sharpe_ratio << "\n";
}

void risk_analysis(Application& app) {
    const string symbol = read_line("Symbol: ");
    if (!app.market().has_symbol(symbol)) {
        cout << "Unknown symbol.\n";
        return;
    }
    const auto closes = app.market().close_series(symbol);
    const auto returns = stats::simple_returns(span<const Price>{closes});
    const auto m = app.risk().compute(span<const double>{returns});
    cout << fixed << setprecision(4);
    print_header("Risk Analysis: " + symbol);
    cout << "Annualized volatility: " << m.annualized_volatility * 100 << " %\n";
    cout << "Sharpe ratio:          " << m.sharpe_ratio << "\n";
    cout << "Sortino ratio:         " << m.sortino_ratio << "\n";
    cout << "Max drawdown:          " << m.max_drawdown * 100 << " %\n";
    cout << "VaR (95%):             " << m.value_at_risk * 100 << " %\n";
    cout << "CVaR (95%):            " << m.conditional_var * 100 << " %\n";
}

void analytics_dashboard(Application& app) {
    const string symbol = read_line("Symbol: ");
    if (!app.market().has_symbol(symbol)) {
        cout << "Unknown symbol.\n";
        return;
    }
    const auto bars   = app.market().history(symbol);
    const auto closes = app.market().close_series(symbol);
    const auto monthly = app.analytics().monthly_returns(bars);
    cout << fixed << setprecision(4);
    print_header("Analytics: " + symbol);
    cout << "Annualized return: "
              << app.analytics().annualized_return(span<const Price>{closes}) * 100
              << " %\n\nRecent monthly returns:\n";
    const size_t show = min<size_t>(6, monthly.size());
    for (size_t i = monthly.size() - show; i < monthly.size(); ++i) {
        cout << "  " << monthly[i].label << "  " << monthly[i].value * 100 << " %\n";
    }
}

void benchmark_report(Application& app) {
    print_header("Performance Benchmark Report");
    const auto symbols = app.market().symbols();
    if (symbols.empty()) {
        cout << "Load market data first.\n";
        return;
    }
    const auto& sym = symbols.front();
    cout << fixed << setprecision(3);

    const double t_close = time_call([&] {
        volatile auto c = app.market().close_series(sym);
        (void)c;
    });
    cout << "Close series extraction (" << sym << "): " << t_close << " ms\n";

    auto strat = StrategyFactory::create(StrategyType::MovingAverageCrossover);
    Backtester bt;
    const auto bt_ms = time_call([&] {
        auto r = bt.run(*strat, app.market().history(sym), sym);
        (void)r;
    });
    cout << "Backtest runtime (" << sym << "): " << bt_ms << " ms\n";

    unordered_map<Symbol, vector<double>> rets;
    for (const auto& s : symbols) {
        const auto c = app.market().close_series(s);
        rets[s] = stats::simple_returns(span<const Price>{c});
    }
    const auto risk_ms = time_call([&] {
        auto b = app.risk().compute_batch(rets, app.pool());
        (void)b;
    });
    cout << "Parallel risk over " << symbols.size() << " symbols ("
              << app.pool().size() << " threads): " << risk_ms << " ms\n";
}

void watchlist(Application& app) {
    static set<string> list;
    cout << "Watchlist commands: add SYM | remove SYM | show\n";
    const string cmd = read_line("> ");
    istringstream iss(cmd);
    string action, sym;
    iss >> action >> sym;
    if (action == "add" && !sym.empty()) {
        const auto [it, inserted] = list.insert(sym);
        cout << (inserted ? "Added " : "Already present: ") << sym << "\n";
    } else if (action == "remove" && !sym.empty()) {
        cout << (list.erase(sym) ? "Removed " : "Not found: ") << sym << "\n";
    } else {
        cout << "Watchlist (" << list.size() << "): ";
        for (const auto& s : list) cout << s << " ";
        cout << "\n";
    }
    (void)app;
}

void undo_trade(Application& app) {
    if (app.orders().undo_last_trade()) {
        cout << "Last trade undone. Cash now " << fixed << setprecision(2)
                  << app.portfolio().cash() << "\n";
    } else {
        cout << "Nothing to undo.\n";
    }
}

}  // namespace

int main() {
    Logger::instance().set_level(LogLevel::Warn);  // keep the menu clean
    Application app;
    app.bootstrap();

    cout << "AlphaForge loaded " << app.market().symbols().size()
              << " symbols from '" << Config::instance().data_directory() << "'.\n";

    for (;;) {
        show_menu();
        const string choice = read_line("\nSelect an option: ");
        if (choice == "1") load_data(app);
        else if (choice == "2") view_market(app);
        else if (choice == "3") portfolio_dashboard(app);
        else if (choice == "4") submit_order(app);
        else if (choice == "5") run_backtest(app);
        else if (choice == "6") risk_analysis(app);
        else if (choice == "7") analytics_dashboard(app);
        else if (choice == "8") benchmark_report(app);
        else if (choice == "9") watchlist(app);
        else if (choice == "10") undo_trade(app);
        else if (choice == "11" || choice == "q") { cout << "Goodbye.\n"; break; }
        else cout << "Invalid option.\n";
    }
    return 0;
}
