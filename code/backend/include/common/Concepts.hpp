#pragma once

#include <concepts>
#include <iterator>
#include <ranges>

using namespace std;

namespace alphaforge {

// Accepts any floating point type. Used to constrain the statistics layer so
// that callers cannot accidentally instantiate it with an integral type and
// silently truncate intermediate results.
template <typename T>
concept FloatingPoint = floating_point<T>;

// Accepts any arithmetic type (integral or floating point).
template <typename T>
concept Arithmetic = integral<T> || floating_point<T>;

// A range whose element type is floating point. Lets algorithms accept a
// vector<double>, a span<const double>, or any view that yields
// doubles without overloading for each container.
template <typename R>
concept FloatingRange =
    ranges::input_range<R> &&
    floating_point<ranges::range_value_t<R>>;

} // namespace alphaforge
