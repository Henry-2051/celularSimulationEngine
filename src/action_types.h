#include <cstddef>
#include "Vec2.hpp"
#include "Materials.h"

#ifndef ACTIONS_TYPES
#define ACTIONS_TYPES
struct SetPixelAction;

struct DrawCircle;

struct DrawLineAction;

struct DrawParallelogramAction;

struct m_IgniteAction;

struct IgnitionAction;

struct ActionPair;

struct IncinerationAction;

struct m_IgniteAction;

struct SetPixelMaterialAndProperties;

using Action = std::variant<SetPixelAction, DrawCircle, DrawLineAction, DrawParallelogramAction, IgnitionAction, IncinerationAction, m_IgniteAction, SetPixelMaterialAndProperties>;

using ActionIncludingPair = std::variant<Action, ActionPair>;

struct Pixel 
{
    uint8_t material;
    uint8_t updateFrame;
    uint8_t properties;
};

using FirePixel = material_properties::ignitionProperties;

struct SetPixelAction
{
    Vec2i pos;
    Pixel pixel_type;
};

struct DrawCircle
{
    Vec2i pos;
    int radius;
    Pixel pixel_type;
};

struct DrawLineAction
{
    Vec2i start;
    Vec2i end;
    int width;
    Pixel pixel_type;
};

struct DrawParallelogramAction
{
    Vec2i cornerTL;
    Vec2i cornerTR;
    Vec2i cornerBL;
    Pixel pixel_type;
};


struct m_IgniteAction
{
    FirePixel fireProperties;
    Vec2i     pos;
};

struct IgnitionAction {
    Vec2i pos;
};

struct IncinerationAction {
    Vec2i pos;
    Pixel pixel;
};


struct SetPixelMaterialAndProperties{
    Vec2i pos;
    uint8_t material;
    uint8_t properties;
};

struct ActionPair {
    std::pair<Action, Action> this_action;
};
#endif