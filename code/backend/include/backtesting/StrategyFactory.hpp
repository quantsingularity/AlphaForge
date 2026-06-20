#pragma once

#include "backtesting/Strategy.hpp"
#include "common/Types.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

using namespace std;

namespace alphaforge {

// Factory pattern. Creates strategy instances from an enum or a case
// insensitive name, returning ownership through a unique_ptr. Centralising
// construction here means the CLI, REST API and tests all build strategies the
// same way and new strategies are registered in one place.
class StrategyFactory {
public:
    [[nodiscard]] static unique_ptr<Strategy> create(StrategyType type);

    // Returns nullptr if the name does not match a known strategy.
    [[nodiscard]] static unique_ptr<Strategy> create(const string& name);

    [[nodiscard]] static optional<StrategyType> parse(const string& name);

    [[nodiscard]] static vector<string> available();
};

} // namespace alphaforge
