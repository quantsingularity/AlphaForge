#include "backtesting/StrategyFactory.hpp"

#include <algorithm>
#include <cctype>

using namespace std;

namespace alphaforge {

namespace {
string to_lower(string s) {
    ranges::transform(s, s.begin(),
                           [](unsigned char c) { return tolower(c); });
    return s;
}
}  // namespace

unique_ptr<Strategy> StrategyFactory::create(StrategyType type) {
    switch (type) {
        case StrategyType::BuyAndHold:
            return make_unique<BuyAndHold>();
        case StrategyType::MovingAverageCrossover:
            return make_unique<MovingAverageCrossover>();
        case StrategyType::Momentum:
            return make_unique<Momentum>();
        case StrategyType::MeanReversion:
            return make_unique<MeanReversion>();
    }
    return nullptr;
}

optional<StrategyType> StrategyFactory::parse(const string& name) {
    const string n = to_lower(name);
    if (n == "buyandhold" || n == "buy_and_hold" || n == "bh") {
        return StrategyType::BuyAndHold;
    }
    if (n == "movingaveragecrossover" || n == "ma" || n == "macrossover" ||
        n == "sma") {
        return StrategyType::MovingAverageCrossover;
    }
    if (n == "momentum" || n == "mom") {
        return StrategyType::Momentum;
    }
    if (n == "meanreversion" || n == "meanrev" || n == "mr") {
        return StrategyType::MeanReversion;
    }
    return nullopt;
}

unique_ptr<Strategy> StrategyFactory::create(const string& name) {
    const auto type = parse(name);
    return type ? create(*type) : nullptr;
}

vector<string> StrategyFactory::available() {
    return {"BuyAndHold", "MovingAverageCrossover", "Momentum", "MeanReversion"};
}

} // namespace alphaforge
