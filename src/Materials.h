#pragma once
#include <cstdint>
#include <map>
#include <sys/types.h>
#include <utility>


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
constexpr uint8_t lava  = 0b00001010;
constexpr uint8_t obsidian = 0b00001011;

constexpr int NumMaterials = 12; 

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
    "steam",
    "lava",
    "obsidian"
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
    steam,
    lava,
    obsidian
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
    steam,
    lava,
    obsidian
};
}

namespace pixel_properties {

enum PixelFlags : uint8_t {
    None = 0,
    OnFire = 1 << 0,
    AlwaysOnFire = 1 << 1,
};

constexpr uint8_t DefaultMaterialProperties[materials::NumMaterials] {
    None,
    None,
    None,
    None,
    None,
    None,
    None,
    None,
    None,
    None,
    OnFire | AlwaysOnFire,
    None,
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
    { static_cast<PixelFlags>(IsSolid | Flammable), 800, 0, {500, 6, true}},

    // oil
    { static_cast<PixelFlags>(CanFall | IsLiquid | Flammable), 900, 260, {1000,30, true}},

    // coal
    { static_cast<PixelFlags>(CanFall | IsPowder | IsSolid | Flammable), 1400, 3000, {2000,2, true}},

    // ash
    { static_cast<PixelFlags>(CanFall | IsPowder | IsSolid), 1300, 1970, nullBurnProperties },

    // steam
    { static_cast<PixelFlags>(IsGas), 9, 60, nullBurnProperties },

    //lava
    { static_cast<PixelFlags>(IsLiquid | CanFall ), 2400, 1470, {999, 20, false} },

    // obsidian 
    { static_cast<PixelFlags>( IsSolid | CanMelt ), 2400, 1470, nullBurnProperties }
};
}

namespace material_reactions {
struct material_property_pair {
    uint8_t material;
    uint8_t properties;
};
inline std::map<std::pair<uint8_t, uint8_t>, std::pair<material_property_pair, material_property_pair>> on_fire_reactions {
        {   
            {materials::oil, materials::water}, 
            {{materials::air, pixel_properties::None}, {materials::steam, pixel_properties::None}}
        },

        {   
            {materials::wood, materials::water}, 
            {{materials::ash, pixel_properties::None}, {materials::steam, pixel_properties::None}}
        },
        {   
            {materials::coal, materials::water}, 
            {{materials::coal, pixel_properties::None}, {materials::steam, pixel_properties::None}}
        },
        {
            {materials::lava, materials::water},
            {{materials::obsidian, pixel_properties::None}, {materials::steam, pixel_properties::None}}
        }
    };
}
