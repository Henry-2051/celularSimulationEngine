#pragma once

#include <iostream>
#include <cassert>
#include <string>

namespace debugAssert {
// This bool is constexpr if supported, else just const
constexpr bool isDebugBuild =
#ifdef DEBUG_BUILD
    true;
#else
    false;
#endif

inline void assertFailiure(std::string message) {
    std::cerr << "Assertion failed: " << message << "\n";
    assert(false);
}
}
