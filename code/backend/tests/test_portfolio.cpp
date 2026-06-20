#include "TestFramework.hpp"

#include "execution/OrderManager.hpp"
#include "portfolio/Portfolio.hpp"

using namespace std;

using namespace alphaforge;

TEST_CASE("position.average_cost_on_add") {
    Position p("AAA");
    p.apply_fill(100, 10.0);   // buy 100 @ 10
    p.apply_fill(100, 12.0);   // buy 100 @ 12
    CHECK_NEAR(p.quantity(), 200.0, 1e-9);
    CHECK_NEAR(p.avg_price(), 11.0, 1e-9);
    CHECK_NEAR(p.realized_pnl(), 0.0, 1e-9);
}

TEST_CASE("position.realized_pnl_on_reduce") {
    Position p("AAA");
    p.apply_fill(100, 10.0);
    p.apply_fill(-40, 15.0);   // sell 40 @ 15, basis 10 -> +200
    CHECK_NEAR(p.quantity(), 60.0, 1e-9);
    CHECK_NEAR(p.avg_price(), 10.0, 1e-9);
    CHECK_NEAR(p.realized_pnl(), 200.0, 1e-9);
}

TEST_CASE("position.flip_long_to_short") {
    Position p("AAA");
    p.apply_fill(100, 10.0);
    p.apply_fill(-150, 12.0);  // close 100 (+200), open -50 @ 12
    CHECK_NEAR(p.quantity(), -50.0, 1e-9);
    CHECK_NEAR(p.avg_price(), 12.0, 1e-9);
    CHECK_NEAR(p.realized_pnl(), 200.0, 1e-9);
}

TEST_CASE("portfolio.cash_and_valuation") {
    Portfolio pf(100000.0);
    pf.execute("AAA", Side::Buy, 100, 50.0);
    CHECK_NEAR(pf.cash(), 100000.0 - 5000.0, 1e-6);
    pf.set_mark("AAA", 55.0);
    CHECK_NEAR(pf.holdings_value(), 5500.0, 1e-6);
    CHECK_NEAR(pf.total_value(), 95000.0 + 5500.0, 1e-6);
    CHECK_NEAR(pf.unrealized_pnl(), 500.0, 1e-6);
}

TEST_CASE("portfolio.undo_restores_state") {
    Portfolio pf(100000.0);
    pf.execute("AAA", Side::Buy, 100, 50.0);
    pf.execute("BBB", Side::Buy, 10, 100.0);
    const double before = pf.cash();
    pf.execute("AAA", Side::Sell, 50, 60.0);
    CHECK(pf.cash() != before);
    CHECK(pf.undo_last_fill());
    CHECK_NEAR(pf.cash(), before, 1e-6);
    auto pos = pf.position("AAA");
    CHECK(pos.has_value());
    CHECK_NEAR(pos->quantity(), 100.0, 1e-9);
}

TEST_CASE("oms.fill_and_undo") {
    Portfolio pf(100000.0);
    OrderManager oms(pf);
    const auto id = oms.submit("AAA", Side::Buy, OrderType::Market, 100);
    CHECK(id == 1);
    auto result = oms.process_next(50.0);
    CHECK(result.has_value());
    CHECK(result->status == OrderStatus::Filled);
    CHECK_NEAR(pf.cash(), 95000.0, 1e-6);
    CHECK(oms.undo_last_trade());
    CHECK_NEAR(pf.cash(), 100000.0, 1e-6);
}

TEST_CASE("oms.limit_not_marketable") {
    Portfolio pf(100000.0);
    OrderManager oms(pf);
    // Buy limit at 40 while reference is 50 -> not marketable, rejected.
    oms.submit("AAA", Side::Buy, OrderType::Limit, 100, 40.0);
    auto result = oms.process_next(50.0);
    CHECK(result.has_value());
    CHECK(result->status == OrderStatus::Rejected);
    CHECK_NEAR(pf.cash(), 100000.0, 1e-6);
}
