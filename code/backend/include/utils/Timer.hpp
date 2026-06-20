#pragma once

#include <chrono>
#include <string>
#include <utility>

using namespace std;

namespace alphaforge {

// Lightweight stopwatch. Call elapsed_ms() as many times as needed; reset()
// restarts the clock. Used by the benchmarking layer and the CLI report.
class Stopwatch {
public:
    using Clock = chrono::steady_clock;

    Stopwatch() : start_(Clock::now()) {}

    void reset() noexcept { start_ = Clock::now(); }

    [[nodiscard]] double elapsed_ms() const noexcept {
        const auto end = Clock::now();
        return chrono::duration<double, milli>(end - start_).count();
    }

    [[nodiscard]] double elapsed_us() const noexcept {
        const auto end = Clock::now();
        return chrono::duration<double, micro>(end - start_).count();
    }

private:
    Clock::time_point start_;
};

// Measure a single invocation and return the result together with the elapsed
// milliseconds. Works for any callable. Move semantics keep the result cheap.
template <typename Callable>
[[nodiscard]] auto time_call(Callable&& fn) {
    Stopwatch sw;
    if constexpr (is_void_v<invoke_result_t<Callable>>) {
        forward<Callable>(fn)();
        return sw.elapsed_ms();
    } else {
        auto result = forward<Callable>(fn)();
        return pair{move(result), sw.elapsed_ms()};
    }
}

} // namespace alphaforge
