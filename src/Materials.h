#pragma once
#include <cstdint>
#include <sys/types.h>


namespace material_properties {
struct ignitionProperties;
}

namespace materials {
constexpr uint8_t air   = 0b00000000;
constexpr uint8_t sand  = 0b00000001;
constexpr uint8_t water = 0b00000010;
constexpr uint8_t stone = 0b00000011;
constexpr uint8_t steel = 0b00000100;
constexpr uint8_t wood  = 0b00000101;
constexpr uint8_t oil   = 0b00000110;
constexpr uint8_t coal  = 0b00000111;
constexpr uint8_t ash   = 0b00001000;
constexpr uint8_t steam = 0b00001001;

constexpr int NumMaterials = 10; 

// Material names for UI display (e.g., ImGui picker)
constexpr const char* materialNames[NumMaterials] = {
    "air",
    "sand",
    "water",
    "stone",
    "steel",
    "wood",
    "oil",
    "coal",
    "ash",
    "steam"
};

constexpr const uint8_t materials[NumMaterials] = {
    air,
    sand,
    water,
    stone,
    steel,
    wood,
    oil,
    coal,
    ash,
    steam
};

constexpr const uint8_t incineration_table[NumMaterials] = {
    air,
    sand,
    steam,
    stone,
    steel,
    ash,
    air,
    air,
    ash,
    steam
};
}

namespace pixel_properties {

enum PixelFlags : uint8_t {
    None = 0,
    OnFire = 1 << 0,
};

};

namespace material_properties {

// Bitflags for pixel/material properties
enum PixelFlags : uint32_t {
    CanFall = 1 << 0,
    IsLiquid = 1 << 1,
    IsPowder = 1 << 2,
    CanMelt = 1 << 3,
    ProducesSteam = 1 << 4,
    IsSolid = 1 << 5,
    ConductsElecticity = 1 << 6 ,
    Flammable = 1 << 7,
    IsGas = 1 << 8,
};
constexpr PixelFlags fallingPowder = static_cast<PixelFlags>(CanFall | IsPowder | IsSolid);

constexpr PixelFlags solidPowder   = static_cast<PixelFlags>(IsPowder | IsSolid);

constexpr PixelFlags fallingLiquid = static_cast<PixelFlags>(CanFall | IsLiquid);


struct ignitionProperties {
    uint16_t burnTime;
    uint8_t  burnPercentageChance; // this is actually the chance for fire to spread from a burning pixel to a neighboring pixel
    bool     requiresAir;
};

struct MaterialProperties {
    PixelFlags flags;
    int density;
    int meltingPoint;
    ignitionProperties burnProperties;

    // Other physical/material properties can go here.
};

// Lookup table for material properties indexed by material ID
// Density is measured in kg / m3
// Melting point is in kelvin

constexpr ignitionProperties nullBurnProperties = {0,0,0};

constexpr MaterialProperties materialLookup[materials::NumMaterials] = {
    // Air
    { static_cast<PixelFlags>(IsGas), 10, 60, nullBurnProperties },

    // Sand
    { static_cast<PixelFlags>(CanFall | IsPowder | IsSolid), 1500, 1970, nullBurnProperties },

    // Water
    { static_cast<PixelFlags>(CanFall | IsLiquid | ConductsElecticity), 1000,   270, nullBurnProperties },

    // Stone
    { static_cast<PixelFlags>(IsSolid | CanMelt), 2600,  1470, nullBurnProperties},

    // Steel
    { static_cast<PixelFlags>(IsSolid | CanMelt | ConductsElecticity), 7600, 1770, nullBurnProperties},

    // wood
    { static_cast<PixelFlags>(IsSolid | Flammable), 800, 0, {1000, 6, true}},

    { static_cast<PixelFlags>(CanFall | IsLiquid | Flammable), 900, 260, {60,30, true}},

    // coal
    { static_cast<PixelFlags>(CanFall | IsPowder | IsSolid | Flammable), 1400, 3000, {2000,2, true}},

    // ash
    { static_cast<PixelFlags>(CanFall | IsPowder | IsSolid), 1300, 1970, nullBurnProperties },

    // steam
    { static_cast<PixelFlags>(IsGas), 9, 60, nullBurnProperties }
};

}
