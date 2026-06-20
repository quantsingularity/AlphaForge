#pragma once

#include "analytics/AnalyticsEngine.hpp"
#include "backtesting/Backtester.hpp"
#include "common/Types.hpp"
#include "execution/OrderManager.hpp"
#include "market/Bar.hpp"
#include "portfolio/Portfolio.hpp"
#include "risk/RiskEngine.hpp"

#include <nlohmann/json.hpp>

#include <span>
#include <string>

using namespace std;

namespace alphaforge {

using nlohmann::json;

inline json to_json(const Bar& b) {
    return json{{"date", b.date},   {"open", b.open}, {"high", b.high},
                {"low", b.low},     {"close", b.close}, {"volume", b.volume}};
}

inline json to_json(const Position& p) {
    return json{{"symbol", p.symbol()},
                {"quantity", p.quantity()},
                {"avg_price", p.avg_price()},
                {"realized_pnl", p.realized_pnl()}};
}

inline json to_json(const Portfolio& pf) {
    json positions = json::array();
    for (const auto& pos : pf.positions()) {
        json j = to_json(pos);
        if (auto m = pf.mark(pos.symbol())) {
            j["mark"]            = *m;
            j["market_value"]    = pos.market_value(*m);
            j["unrealized_pnl"]  = pos.unrealized_pnl(*m);
        }
        positions.push_back(j);
    }
    json weights = json::array();
    for (const auto& w : pf.weights()) {
        weights.push_back(json{{"symbol", w.symbol}, {"weight", w.weight}});
    }
    return json{{"cash", pf.cash()},
                {"initial_cash", pf.initial_cash()},
                {"holdings_value", pf.holdings_value()},
                {"total_value", pf.total_value()},
                {"realized_pnl", pf.realized_pnl()},
                {"unrealized_pnl", pf.unrealized_pnl()},
                {"gross_exposure", pf.gross_exposure()},
                {"net_exposure", pf.net_exposure()},
                {"positions", positions},
                {"weights", weights}};
}

inline json to_json(const Fill& f) {
    return json{{"symbol", f.symbol},
                {"side", string{as_string(f.side)}},
                {"quantity", f.quantity},
                {"price", f.price},
                {"cash_delta", f.cash_delta},
                {"timestamp", f.timestamp}};
}

inline json to_json(const RiskMetrics& m) {
    json j{{"annualized_volatility", m.annualized_volatility},
           {"sharpe_ratio", m.sharpe_ratio},
           {"sortino_ratio", m.sortino_ratio},
           {"max_drawdown", m.max_drawdown},
           {"value_at_risk", m.value_at_risk},
           {"conditional_var", m.conditional_var}};
    if (m.beta) {
        j["beta"] = *m.beta;
    } else {
        j["beta"] = nullptr;
    }
    return j;
}

inline json to_json(const BacktestMetrics& m) {
    return json{{"total_return", m.total_return},
                {"cagr", m.cagr},
                {"annualized_volatility", m.annualized_volatility},
                {"sharpe_ratio", m.sharpe_ratio},
                {"sortino_ratio", m.sortino_ratio},
                {"max_drawdown", m.max_drawdown},
                {"win_rate", m.win_rate},
                {"profit_factor", m.profit_factor},
                {"num_trades", m.num_trades}};
}

inline json to_json(const BacktestResult& r) {
    json curve = json::array();
    for (const auto& p : r.equity_curve) {
        curve.push_back(json{{"date", p.date},
                             {"equity", p.equity},
                             {"target_weight", p.target_weight}});
    }
    return json{{"strategy", r.strategy},
                {"symbol", r.symbol},
                {"equity_curve", curve},
                {"metrics", to_json(r.metrics)},
                {"benchmark", to_json(r.benchmark)}};
}

inline json to_json(const SeriesPoint& p) {
    return json{{"label", p.label}, {"value", p.value}};
}

inline json series_to_json(const vector<SeriesPoint>& series) {
    json arr = json::array();
    for (const auto& p : series) {
        arr.push_back(to_json(p));
    }
    return arr;
}

} // namespace alphaforge
