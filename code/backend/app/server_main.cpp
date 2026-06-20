// AlphaForge REST API server.
//
// Exposes the market data, portfolio, execution, risk, analytics and
// backtesting engines over a small JSON HTTP API and serves the built React
// frontend as static files. Run with an optional port argument:
//
//     alphaforge_server [port] [data_dir] [frontend_dir]

#include "core/Application.hpp"
#include "utils/Logger.hpp"
#include "utils/Statistics.hpp"

#include "JsonSupport.hpp"

#include <httplib.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

using namespace alphaforge;

namespace {

void add_cors(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

void send_json(httplib::Response& res, const json& body, int status = 200) {
    add_cors(res);
    res.status = status;
    res.set_content(body.dump(), "application/json");
}

void send_error(httplib::Response& res, const string& message, int status) {
    send_json(res, json{{"error", message}}, status);
}

optional<double> query_double(const httplib::Request& req, const string& key) {
    if (!req.has_param(key)) {
        return nullopt;
    }
    try {
        return stod(req.get_param_value(key));
    } catch (...) {
        return nullopt;
    }
}

optional<int> query_int(const httplib::Request& req, const string& key) {
    if (!req.has_param(key)) {
        return nullopt;
    }
    try {
        return stoi(req.get_param_value(key));
    } catch (...) {
        return nullopt;
    }
}

}  // namespace

int main(int argc, char** argv) {
    Logger::instance().set_level(LogLevel::Info);

    int port = 8080;
    if (argc > 1) {
        try {
            port = stoi(argv[1]);
        } catch (...) {
        }
    } else if (const char* env = getenv("ALPHAFORGE_PORT")) {
        try {
            port = stoi(env);
        } catch (...) {
        }
    }

    const string data_dir = (argc > 2) ? argv[2] : "data";
    const string web_dir  = (argc > 3) ? argv[3] : "frontend";
    Config::instance().set_data_directory(data_dir);

    Application app;
    const size_t loaded = app.bootstrap();
    Logger::instance().info("bootstrap loaded " + to_string(loaded) + " bars");

    httplib::Server server;

    server.Options(R"(/.*)", [](const httplib::Request&, httplib::Response& res) {
        add_cors(res);
        res.status = 204;
    });

    // Health and metadata.
    server.Get("/api/health", [&](const httplib::Request&, httplib::Response& res) {
        send_json(res, json{{"status", "ok"},
                            {"version", Application::version()},
                            {"symbols_loaded", app.market().symbols().size()},
                            {"threads", app.pool().size()}});
    });

    server.Get("/api/strategies", [&](const httplib::Request&, httplib::Response& res) {
        send_json(res, json{{"strategies", StrategyFactory::available()}});
    });

    server.Get("/api/symbols", [&](const httplib::Request&, httplib::Response& res) {
        json arr = json::array();
        for (const auto& s : app.market().symbols()) {
            arr.push_back(json{{"symbol", s},
                               {"bars", app.market().bar_count(s)}});
        }
        send_json(res, json{{"symbols", arr}});
    });

    // Market data for a symbol, optionally limited to the last N bars.
    server.Get(R"(/api/market/([A-Za-z0-9_.\-]+))",
               [&](const httplib::Request& req, httplib::Response& res) {
                   const string symbol = req.matches[1];
                   if (!app.market().has_symbol(symbol)) {
                       send_error(res, "unknown symbol", 404);
                       return;
                   }
                   auto hist = app.market().history(symbol);
                   size_t start = 0;
                   if (auto limit = query_int(req, "limit");
                       limit && *limit > 0 &&
                       static_cast<size_t>(*limit) < hist.size()) {
                       start = hist.size() - static_cast<size_t>(*limit);
                   }
                   json bars = json::array();
                   for (size_t i = start; i < hist.size(); ++i) {
                       bars.push_back(to_json(hist[i]));
                   }
                   send_json(res, json{{"symbol", symbol}, {"bars", bars}});
               });

    // Portfolio snapshot.
    server.Get("/api/portfolio", [&](const httplib::Request&, httplib::Response& res) {
        scoped_lock lock(app.trade_mutex());
        json body = to_json(app.portfolio());
        json blotter = json::array();
        for (const auto& f : app.portfolio().blotter()) {
            blotter.push_back(to_json(f));
        }
        body["blotter"]        = blotter;
        body["pending_orders"] = app.orders().pending_count();
        body["executed_orders"] = app.orders().executed_count();
        send_json(res, body);
    });

    // Submit and immediately process an order against the latest close.
    server.Post("/api/order", [&](const httplib::Request& req, httplib::Response& res) {
        json body;
        try {
            body = json::parse(req.body);
        } catch (...) {
            send_error(res, "invalid JSON body", 400);
            return;
        }
        const string symbol = body.value("symbol", "");
        const string side_s = body.value("side", "buy");
        const string type_s = body.value("type", "market");
        const double quantity    = body.value("quantity", 0.0);

        if (symbol.empty() || quantity <= 0.0) {
            send_error(res, "symbol and positive quantity are required", 400);
            return;
        }
        if (!app.market().has_symbol(symbol)) {
            send_error(res, "unknown symbol", 404);
            return;
        }
        auto last = app.market().latest_bar(symbol);
        if (!last) {
            send_error(res, "no price available for symbol", 409);
            return;
        }

        Side side = (side_s == "sell" || side_s == "SELL") ? Side::Sell : Side::Buy;
        OrderType type = OrderType::Market;
        if (type_s == "limit" || type_s == "LIMIT") type = OrderType::Limit;
        else if (type_s == "stop" || type_s == "STOP") type = OrderType::Stop;

        optional<Price> limit_price;
        optional<Price> stop_price;
        if (body.contains("limit_price") && !body["limit_price"].is_null())
            limit_price = body["limit_price"].get<double>();
        if (body.contains("stop_price") && !body["stop_price"].is_null())
            stop_price = body["stop_price"].get<double>();

        scoped_lock lock(app.trade_mutex());
        const auto id = app.orders().submit(symbol, side, type, quantity, limit_price,
                                            stop_price);
        const auto result = app.orders().process_next(last->close);

        json jr{{"order_id", id}};
        if (result) {
            jr["status"]     = string{as_string(result->status)};
            jr["reason"]     = result->reason;
            if (result->fill_price) jr["fill_price"] = *result->fill_price;
        }
        json out{{"execution", jr}, {"portfolio", to_json(app.portfolio())}};
        send_json(res, out);
    });

    // Undo the most recent executed trade.
    server.Post("/api/undo", [&](const httplib::Request&, httplib::Response& res) {
        scoped_lock lock(app.trade_mutex());
        const bool ok = app.orders().undo_last_trade();
        send_json(res, json{{"undone", ok}, {"portfolio", to_json(app.portfolio())}});
    });

    // Risk metrics for a single symbol.
    server.Get(R"(/api/risk/([A-Za-z0-9_.\-]+))",
               [&](const httplib::Request& req, httplib::Response& res) {
                   const string symbol = req.matches[1];
                   if (!app.market().has_symbol(symbol)) {
                       send_error(res, "unknown symbol", 404);
                       return;
                   }
                   const double confidence = query_double(req, "confidence").value_or(0.95);
                   const auto closes = app.market().close_series(symbol);
                   const auto returns = stats::simple_returns(span<const Price>{closes});

                   optional<span<const double>> bench;
                   vector<double> bench_returns;
                   if (req.has_param("benchmark")) {
                       const string b = req.get_param_value("benchmark");
                       if (app.market().has_symbol(b)) {
                           const auto bc = app.market().close_series(b);
                           bench_returns = stats::simple_returns(span<const Price>{bc});
                           const size_t n = min(returns.size(), bench_returns.size());
                           if (n > 0) {
                               bench = span<const double>{
                                   bench_returns.data() + (bench_returns.size() - n), n};
                           }
                       }
                   }
                   const auto metrics = app.risk().compute(
                       span<const double>{returns}, bench, confidence);
                   json body = to_json(metrics);
                   body["symbol"]     = symbol;
                   body["confidence"] = confidence;
                   body["samples"]    = returns.size();
                   send_json(res, body);
               });

    // Risk metrics across all symbols, computed in parallel.
    server.Get("/api/risk", [&](const httplib::Request& req, httplib::Response& res) {
        const double confidence = query_double(req, "confidence").value_or(0.95);
        unordered_map<Symbol, vector<double>> returns_by_symbol;
        for (const auto& s : app.market().symbols()) {
            const auto closes = app.market().close_series(s);
            returns_by_symbol[s] = stats::simple_returns(span<const Price>{closes});
        }
        const auto batch = app.risk().compute_batch(returns_by_symbol, app.pool(),
                                                    confidence);
        vector<pair<string, RiskMetrics>> ordered(batch.begin(),
                                                                 batch.end());
        ranges::sort(ordered, {}, &pair<string, RiskMetrics>::first);
        json arr = json::array();
        for (const auto& [symbol, metrics] : ordered) {
            json j = to_json(metrics);
            j["symbol"] = symbol;
            arr.push_back(j);
        }
        send_json(res, json{{"confidence", confidence}, {"risk", arr}});
    });

    // Analytics for a single symbol.
    server.Get(R"(/api/analytics/([A-Za-z0-9_.\-]+))",
               [&](const httplib::Request& req, httplib::Response& res) {
                   const string symbol = req.matches[1];
                   if (!app.market().has_symbol(symbol)) {
                       send_error(res, "unknown symbol", 404);
                       return;
                   }
                   const auto window = static_cast<size_t>(
                       query_int(req, "window").value_or(20));
                   const auto bars   = app.market().history(symbol);
                   const auto closes = app.market().close_series(symbol);

                   json body{
                       {"symbol", symbol},
                       {"annualized_return", app.analytics().annualized_return(
                                                 span<const Price>{closes})},
                       {"monthly_returns",
                        series_to_json(app.analytics().monthly_returns(bars))},
                       {"weekly_returns",
                        series_to_json(app.analytics().weekly_returns(bars))},
                       {"rolling_volatility",
                        series_to_json(app.analytics().rolling_volatility(bars, window))},
                       {"rolling_sharpe",
                        series_to_json(app.analytics().rolling_sharpe(bars, window))},
                       {"window", window}};
                   send_json(res, body);
               });

    // Run a backtest.
    server.Post("/api/backtest", [&](const httplib::Request& req, httplib::Response& res) {
        json body;
        try {
            body = json::parse(req.body);
        } catch (...) {
            send_error(res, "invalid JSON body", 400);
            return;
        }
        const string symbol   = body.value("symbol", "");
        const string strategy = body.value("strategy", "BuyAndHold");
        if (!app.market().has_symbol(symbol)) {
            send_error(res, "unknown symbol", 404);
            return;
        }
        auto strat = StrategyFactory::create(strategy);
        if (!strat) {
            send_error(res, "unknown strategy", 400);
            return;
        }
        BacktestConfig cfg;
        cfg.initial_capital      = body.value("initial_capital", Config::instance().initial_capital());
        cfg.transaction_cost_bps = body.value("transaction_cost_bps",
                                              Config::instance().transaction_cost_bps());
        cfg.slippage_bps         = body.value("slippage_bps",
                                             Config::instance().slippage_bps());

        Backtester bt(cfg);
        const auto result = bt.run(*strat, app.market().history(symbol), symbol);
        send_json(res, to_json(result));
    });

    // Static frontend. Mount the build directory if present and fall back to
    // index.html for client side routes.
    const filesystem::path web_path{web_dir};
    if (filesystem::exists(web_path)) {
        server.set_mount_point("/", web_dir);
        Logger::instance().info("serving frontend from " + web_dir);
    }
    server.set_error_handler([&](const httplib::Request& req, httplib::Response& res) {
        if (res.status == 404 && req.path.rfind("/api", 0) != 0) {
            const filesystem::path index = web_path / "index.html";
            if (filesystem::exists(index)) {
                ifstream in(index, ios::binary);
                string html((istreambuf_iterator<char>(in)),
                                 istreambuf_iterator<char>());
                res.status = 200;
                res.set_content(html, "text/html");
            }
        }
    });

    Logger::instance().info("AlphaForge API listening on http://0.0.0.0:" +
                            to_string(port));
    if (!server.listen("0.0.0.0", port)) {
        Logger::instance().error("failed to bind port " + to_string(port));
        return 1;
    }
    return 0;
}
