#pragma once

#include <cmath>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

// A tiny zero dependency test framework. Tests register themselves through the
// TEST_CASE macro; run_all() executes them and reports a summary. Chosen over a
// vendored framework to keep the build self contained and the intent obvious.

namespace aftest {

struct TestCase {
    string name;
    function<void()> fn;
};

inline vector<TestCase>& registry() {
    static vector<TestCase> tests;
    return tests;
}

inline int& failure_count() {
    static int n = 0;
    return n;
}

struct Registrar {
    Registrar(const string& name, function<void()> fn) {
        registry().push_back({name, move(fn)});
    }
};

struct AssertionError {
    string message;
};

inline void check(bool condition, const string& expr, const char* file, int line) {
    if (!condition) {
        throw AssertionError{string(file) + ":" + to_string(line) +
                             "  expected: " + expr};
    }
}

inline void check_near(double a, double b, double tol, const char* file, int line) {
    if (isnan(a) || isnan(b) || fabs(a - b) > tol) {
        throw AssertionError{string(file) + ":" + to_string(line) +
                             "  expected " + to_string(a) + " ~= " +
                             to_string(b) + " (tol " + to_string(tol) + ")"};
    }
}

inline int run_all() {
    int passed = 0;
    for (const auto& test : registry()) {
        try {
            test.fn();
            cout << "[ PASS ] " << test.name << "\n";
            ++passed;
        } catch (const AssertionError& e) {
            cout << "[ FAIL ] " << test.name << "\n         " << e.message << "\n";
            ++failure_count();
        } catch (const exception& e) {
            cout << "[ ERROR] " << test.name << " threw: " << e.what() << "\n";
            ++failure_count();
        }
    }
    cout << "\n" << passed << " passed, " << failure_count() << " failed, "
              << registry().size() << " total\n";
    return failure_count() == 0 ? 0 : 1;
}

}  // namespace aftest

#define AF_CONCAT_(a, b) a##b
#define AF_CONCAT(a, b) AF_CONCAT_(a, b)
#define TEST_CASE(name)                                                    \
    static void AF_CONCAT(af_test_, __LINE__)();                           \
    static ::aftest::Registrar AF_CONCAT(af_reg_, __LINE__)(              \
        name, &AF_CONCAT(af_test_, __LINE__));                            \
    static void AF_CONCAT(af_test_, __LINE__)()

#define CHECK(cond) ::aftest::check((cond), #cond, __FILE__, __LINE__)
#define CHECK_NEAR(a, b, tol) ::aftest::check_near((a), (b), (tol), __FILE__, __LINE__)
