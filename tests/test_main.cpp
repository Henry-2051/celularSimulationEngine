#define CATCH_CONFIG_MAIN  // This tells Catch2 to provide a main() function.
#include <catch2/catch_test_macros.hpp>

// A sample test case for demonstration.
TEST_CASE("Addition works correctly", "[math]") {
    int a = 2;
    int b = 3;
    int sum = a + b;
    REQUIRE(sum == 5);
}
