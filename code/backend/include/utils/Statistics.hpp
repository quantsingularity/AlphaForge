#pragma once

#include "common/Concepts.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <optional>
#include <span>
#include <stdexcept>
#include <vector>

using namespace std;

namespace alphaforge::stats {

// All functions here are constrained on FloatingPoint so an accidental
// integral instantiation is a compile error rather than a silent truncation.

template <FloatingPoint T>
[[nodiscard]] T sum(span<const T> xs) noexcept {
    return accumulate(xs.begin(), xs.end(), T{0});
}

template <FloatingPoint T>
[[nodiscard]] optional<T> mean(span<const T> xs) noexcept {
    if (xs.empty()) {
        return nullopt;
    }
    return sum(xs) / static_cast<T>(xs.size());
}

// Sample variance (n - 1 denominator). Returns nullopt for fewer than two
// observations, where sample variance is undefined.
template <FloatingPoint T>
[[nodiscard]] optional<T> variance(span<const T> xs) noexcept {
    if (xs.size() < 2) {
        return nullopt;
    }
    const T m = *mean(xs);
    T acc{0};
    for (const T x : xs) {
        const T d = x - m;
        acc += d * d;
    }
    return acc / static_cast<T>(xs.size() - 1);
}

template <FloatingPoint T>
[[nodiscard]] optional<T> stdev(span<const T> xs) noexcept {
    const auto v = variance(xs);
    return v ? optional<T>{sqrt(*v)} : nullopt;
}

// Population standard deviation (n denominator). Used where the series is the
// full population rather than a sample.
template <FloatingPoint T>
[[nodiscard]] optional<T> stdev_population(span<const T> xs) noexcept {
    if (xs.empty()) {
        return nullopt;
    }
    const T m = *mean(xs);
    T acc{0};
    for (const T x : xs) {
        const T d = x - m;
        acc += d * d;
    }
    return sqrt(acc / static_cast<T>(xs.size()));
}

// Sample covariance between two equally sized series.
template <FloatingPoint T>
[[nodiscard]] optional<T> covariance(span<const T> a,
                                          span<const T> b) noexcept {
    if (a.size() != b.size() || a.size() < 2) {
        return nullopt;
    }
    const T ma = *mean(a);
    const T mb = *mean(b);
    T acc{0};
    for (size_t i = 0; i < a.size(); ++i) {
        acc += (a[i] - ma) * (b[i] - mb);
    }
    return acc / static_cast<T>(a.size() - 1);
}

template <FloatingPoint T>
[[nodiscard]] optional<T> correlation(span<const T> a,
                                           span<const T> b) noexcept {
    const auto cov = covariance(a, b);
    const auto sa  = stdev(a);
    const auto sb  = stdev(b);
    if (!cov || !sa || !sb || *sa == T{0} || *sb == T{0}) {
        return nullopt;
    }
    return *cov / (*sa * *sb);
}

// Downside deviation: standard deviation of returns below a threshold (often
// the minimum acceptable return). Used by the Sortino ratio.
template <FloatingPoint T>
[[nodiscard]] optional<T> downside_deviation(span<const T> xs,
                                                  T threshold = T{0}) noexcept {
    if (xs.empty()) {
        return nullopt;
    }
    T acc{0};
    size_t count = 0;
    for (const T x : xs) {
        if (x < threshold) {
            const T d = x - threshold;
            acc += d * d;
            ++count;
        }
    }
    if (count == 0) {
        return T{0};
    }
    return sqrt(acc / static_cast<T>(count));
}

// Linear interpolated percentile, q in [0, 1]. Copies and sorts internally so
// the caller's data is left untouched.
template <FloatingPoint T>
[[nodiscard]] optional<T> percentile(span<const T> xs, T q) {
    if (xs.empty() || q < T{0} || q > T{1}) {
        return nullopt;
    }
    vector<T> sorted(xs.begin(), xs.end());
    ranges::sort(sorted);
    if (sorted.size() == 1) {
        return sorted.front();
    }
    const T pos   = q * static_cast<T>(sorted.size() - 1);
    const auto lo = static_cast<size_t>(floor(pos));
    const auto hi = static_cast<size_t>(ceil(pos));
    const T frac  = pos - static_cast<T>(lo);
    return sorted[lo] + (sorted[hi] - sorted[lo]) * frac;
}

// Simple period over period returns from a price series. Output size is
// prices.size() - 1.
template <FloatingPoint T>
[[nodiscard]] vector<T> simple_returns(span<const T> prices) {
    vector<T> out;
    if (prices.size() < 2) {
        return out;
    }
    out.reserve(prices.size() - 1);
    for (size_t i = 1; i < prices.size(); ++i) {
        if (prices[i - 1] == T{0}) {
            out.push_back(T{0});
        } else {
            out.push_back(prices[i] / prices[i - 1] - T{1});
        }
    }
    return out;
}

// Log returns from a price series.
template <FloatingPoint T>
[[nodiscard]] vector<T> log_returns(span<const T> prices) {
    vector<T> out;
    if (prices.size() < 2) {
        return out;
    }
    out.reserve(prices.size() - 1);
    for (size_t i = 1; i < prices.size(); ++i) {
        if (prices[i - 1] > T{0} && prices[i] > T{0}) {
            out.push_back(log(prices[i] / prices[i - 1]));
        } else {
            out.push_back(T{0});
        }
    }
    return out;
}

} // namespace alphaforge::stats
