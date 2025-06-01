#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Window/Window.hpp>
#include "PixelGrid.hpp"
#include "Vec2.hpp"
#include "UserInputOptions.h"
#include "Materials.h"

class Game
{
    sf::RenderWindow         m_window;
    sf::Font                 font;
    sf::Text                 m_text;
    sf::Clock                m_deltaClock;

    int m_fps                = 60;
    int m_scale              = 8;
    int m_width = 320;
    int m_height = 180;

    bool m_running = true;

    bool m_left_button_pressed = false;
    bool m_prev_left_button_pressed= false;

    int m_windowHeight       =m_height* m_scale;
    int m_windowWidth        =m_width *m_scale;

    sf::Texture m_texture;
    sf::Sprite m_sprite;
    PixelGrid                m_pixelGrid;
    
    Pixel m_drawPixelType = Pixel{materials::stone};
    int m_drawMethod = inputModes::paintBrush;

    
    float m_drawLineWidth = 1.0f;
    bool m_holdAction = false;
    Vec2f m_previousMousePos = Vec2f{0.0f, 0.0f};

    Vec2i m_dragLineStartSimulationPos = m_previousMousePos / m_scale; 
    
    
    void init(const std::string & config);
    void sUserInput();
    void sRender();
    void update();

    void heldButtons();
    void paintBrush();
    void holdAndDragLine();
    void drawCircle();
    void sGui();

public:
    Game(const std::string & config);

    void run();
};
