#pragma once

// function to check whether a given bit flag is empty ie all the bits are zero
// assuming internal storage of uint32_t

#include <cstdint>
namespace bitop {
    inline bool empty(uint32_t flag)
    {
        return flag == 0;
    };

    inline bool flag_has_mask(uint32_t flag, uint32_t mask) {
        return (flag & mask) == mask;
    }
}
