#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Window/Window.hpp>
#include <chrono>
#include "PixelGrid.hpp"
#include "Vec2.hpp"
#include "UserInputOptions.h"
#include "Materials.h"

class Game
{
    enum DrawParallelogramStates : uint8_t {
        WaitingForStart = 0,
        HeldDown1 = 1,
        WaitingForSecondClick = 2,
        DrawingShape = 3,
    };

    Vec2i m_firstParallelogramPoint;
    Vec2i m_secondParallelogramPoint;
    Vec2i m_thirdParallelogramPoint;

    DrawParallelogramStates m_lastParallelogramState = WaitingForStart;
    DrawParallelogramStates m_currentParallelogramState = WaitingForStart;

    sf::RenderWindow         m_window;
    sf::Font                 font;
    sf::Text                 m_text;
    sf::Clock                m_deltaClock;

    using clock = std::chrono::steady_clock;
    using seconds = std::chrono::duration<double>;
    
    std::chrono::steady_clock::time_point previousTime;
    int m_fps                = 60;
    int m_scale              = 1;
    size_t m_width = 1600;   // pixel grid width and height this should change at runtime
    size_t m_height = 900;
    int m_frameCount = 1;

    bool m_running = true;

    bool m_left_button_pressed = false;
    bool m_prev_left_button_pressed= false;

    size_t m_windowHeight  = m_height * m_scale;  // the size of the actual window
    size_t m_windowWidth   = m_width * m_scale;   // 

    sf::Texture m_texture;
    sf::Sprite m_sprite;
    PixelGrid                m_pixelGrid;
    
    Pixel m_drawPixelType = Pixel{materials::stone, 0, 0};
    int m_drawMethod = inputModes::paintBrush;

    
    int m_drawLineWidth = 1;
    int m_drawCircleRadius = 4;
    bool m_holdAction = false;
    Vec2st m_previousMousePos = Vec2st{m_width / 2, m_height / 2};
    int m_paintBrushWidth = 1;

    Vec2i m_dragLineStartSimulationPos = m_previousMousePos / m_scale; 

    Vec2i m_windowPosition = {0,0}; // this is the top left cornet of the window view into the pixel grid dynamic
    // m_scale is the other dynamic parameter
    const Vec2st m_windowSize = {m_windowWidth, m_windowHeight};
    
    
    void init(const std::string & config);
    void sUserInput();
    void sRender();
    void update();

    void heldButtons();
    void paintBrush();
    void holdAndDragLine();
    void drawCircle();
    void sGui();

    void drawParallelogram();

    bool hasUserLeftClicked();
    bool hasUserLeftReleased();

    void tryIgnite();
public:
    Game(const std::string & config);

    void run();
};
