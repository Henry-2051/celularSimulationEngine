#include "../src/BitOperationHelpers.hpp"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>


template <typename UInt>
void printBinary(UInt value) {
    static_assert(std::is_unsigned<UInt>::value, "Must be an unsigned integer");
    std::stringstream ss;
    int st_size =  sizeof(value);
    for (int i = 0; i < (st_size * 8); i++) {
        if (value % 2 == 1) {
            ss << "1";
            value -= 1;
            value /= 2;
            continue;
        } 
        ss << "0";
        value /= 2;
    }
    std::string s = ss.str();
    std::reverse(s.begin(), s.end());
    std::cout << s << "\n";
}

int main (int argc, char *argv[]) {
    uint8_t my_binary = 0b00010101;
    printBinary(my_binary);
    std::cout << bitop::check_nth_bit(my_binary, 9);
    return 0;
}
