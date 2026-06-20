#include "TestFramework.hpp"

using namespace std;

// All test cases self register through the TEST_CASE macro in the translation
// units linked into this binary. The runner simply executes them and returns a
// non zero exit code if any failed, which is what CTest checks.

int main() {
    return aftest::run_all();
}
