#pragma once
#include <cstdint>

namespace materials {
    constexpr uint8_t Air   = 0b00000000;
    constexpr uint8_t Sand  = 0b00000001;
    constexpr uint8_t Water = 0b00000010;
    constexpr uint8_t Stone = 0b00000011;
    constexpr uint8_t Steel = 0b00000100;

    constexpr int NumMaterials = 5; 

    // Material names for UI display (e.g., ImGui picker)
    constexpr const char* MaterialNames[NumMaterials] = {
        "Air",
        "Sand",
        "Water",
        "Stone",
        "Steel"
    };
}

namespace properties {

    // Bitflags for pixel/material properties
    enum PixelFlags : uint32_t {
        CanFall = 1 << 0,
        IsLiquid = 1 << 1,
        IsPowder = 1 << 2,
        CanMelt = 1 << 3,
        ProducesSteam = 1 << 4,
        IsSolid = 1 << 5,
        ConductsElecticity = 1 << 6 
        // Add more flags as needed
    };

    struct MaterialProperties {
        PixelFlags flags;
        float density;
        float meltingPoint;
        // Other physical/material properties can go here.
    };

    // Lookup table for material properties indexed by material ID
    // Density is measured in kg / m3
    // Melting point is in kelvin
    constexpr MaterialProperties MaterialLookup[materials::NumMaterials] = {
        // Air
        { PixelFlags(0), 1.2f, 60.0f },

        // Sand
        { static_cast<PixelFlags>(CanFall | IsPowder | IsSolid), 1500.0f, 1970.0f },

        // Water
        { static_cast<PixelFlags>(CanFall | IsLiquid | ConductsElecticity), 1000.0f,   270.0f    },

        // Stone
        { static_cast<PixelFlags>(IsSolid | CanMelt), 2600.0f,  1470.0f },

        // Steel
        { static_cast<PixelFlags>(IsSolid | CanMelt | ConductsElecticity), 7600.0f, 1770.0f}
    };

}
