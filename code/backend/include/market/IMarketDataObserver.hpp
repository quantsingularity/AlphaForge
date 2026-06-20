#pragma once

#include "common/Types.hpp"
#include "market/Bar.hpp"

using namespace std;

namespace alphaforge {

// Observer side of the market data subject. Components that need to react to a
// new bar (for example a portfolio re marking its positions, or a streaming
// dashboard) implement this interface and subscribe to the engine.
class IMarketDataObserver {
public:
    virtual ~IMarketDataObserver() = default;
    virtual void on_bar(const Symbol& symbol, const Bar& bar) = 0;
};

} // namespace alphaforge
