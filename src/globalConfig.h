#include <cstddef>
#include "PixelGrid.hpp"
#include "Materials.h"
#include <random>
#include "Vec2.hpp"
#include "UserInputOptions.h"
#include <functional>
#include <map>
#include "configHelp.h"


struct LoadedConfig {
    int fps = 60;
    int scale = 1;
    std::string rendering_engine = "SFML2";

    size_t pGrid_width = 1600;
    size_t pGrid_height = 900;

    size_t window_width = 1600;
    size_t window_height = 900;

};

inline void printConfig(const LoadedConfig& cfg, std::ostream& os = std::cout)
{
    os << "LoadedConfig:\n";
    os << "  fps              = " << cfg.fps              << "\n";
    os << "  scale            = " << cfg.scale            << "\n";
    os << "  rendering_engine = " << cfg.rendering_engine << "\n";
    os << "  pGrid_width      = " << cfg.pGrid_width      << "\n";
    os << "  pGrid_height     = " << cfg.pGrid_height     << "\n";
}
using FieldPtr = std::variant<
    int LoadedConfig::*,
    size_t LoadedConfig::*,
    bool LoadedConfig::*,
    std::string LoadedConfig::*
>;

template<typename T>
std::function<void(LoadedConfig&, const std::string&)>
make_setter(T LoadedConfig::* memberPtr)
{
    return [memberPtr](LoadedConfig& cfg, const std::string& val) {
        cfg.*memberPtr = config_help::parseValue<T>(val);
    };
}

struct SFML2State {
    sf::Texture texture;
    sf::Sprite sprite;
    size_t window_height;
    size_t window_width;

    size_t draw_texture_width;
    size_t draw_texture_height;
    sf::RenderWindow window;
    sf::Clock delta_clock;
};

struct OpenGLState {};

struct ParallelogramState {
    enum DrawParallelogramStates : uint8_t {
        WaitingForStart = 0,
        HeldDown = 1,
        WaitingForSecondClick = 2,
        DrawingShape = 3,
    };
    Vec2i first_parallelogram_point;
    Vec2i second_parallelogram_point;
    Vec2i third_parallelogram_point;

    DrawParallelogramStates last_parallelogram_state{WaitingForStart};
    DrawParallelogramStates current_parallelogram_state{WaitingForStart};
};

struct GameState {
    LoadedConfig persistent_state;
    SFML2State sfml2_state;
    OpenGLState opengl_state;

    ParallelogramState parallelogramState;

    bool running = true;
    int frame_count {};
    bool left_button_pressed = false;
    bool right_button_pressed = false;
    bool prev_left_button_pressed = false;
    Vec2st previous_mouse_position;
    Vec2st drag_line_start_simulation_pos;

    PixelGrid pixel_grid;
    Pixel draw_pixel_type;
    int draw_method;

    int draw_line_width = 1;
    int draw_circle_radius = 4;
    bool hold_action = false;
    int paint_brush_width = 1;

    Vec2i window_position = {0,0};
};

// GameState createInitialGameState(const LoadedConfig & lc) {
//     GameState gs {};
//     gs.window_height = lc.pGrid_height * lc.scale;
//     gs.window_width = lc.pGrid_width * lc.scale;
// }
