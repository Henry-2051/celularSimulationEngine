#include <SFML/Graphics/Texture.hpp>
#include <iostream>
#include <string>
#include <variant>
#include "Game.h"
#include "UserInputOptions.h"
#include "Vec2.hpp"
#include "imgui-SFML.h"
#include "imgui.h"

Game::Game(const std::string& config) : m_pixelGrid(m_width, m_height)
{
    init(config);
}

void
Game::init(const std::string & path)
{
    // TODO add code here to load config from config.txt

    m_window.create(sf::VideoMode(m_windowWidth, m_windowHeight),
                    "Falling sand sim");
    m_window.setFramerateLimit(m_fps);

    ImGui::SFML::Init(m_window);

    ImGui::GetStyle().ScaleAllSizes(2.0f);
    ImGui::GetIO().FontGlobalScale = 2.0f;
    /*m_pixelGrid = PixelGrid(m_windowWidth / m_scale, m_windowHeight / m_scale);*/

    m_texture.create(m_width, m_height);

    /*std::cout << "Making sprite\n";*/
    // Create a sprite to display the texture
    m_sprite.setTexture(m_texture);
    m_sprite.setScale(static_cast<float>(m_scale), static_cast<float>(m_scale)); // Scale up
    /*std::cout << "sprite made\n";*/
}

void
Game::run() 
{
    while (m_running) 
    {
        /*std::cout << "Running program\n";*/
        m_pixelGrid.update();       
        /*std::cout << "updated\n";*/
        ImGui::SFML::Update(m_window, m_deltaClock.restart());
        sUserInput();
        heldButtons();
        sRender();
    }
}

void
Game::sRender()
{
    m_window.clear();
    /*std::cout << "in rendering function\n";*/

    m_texture.update(m_pixelGrid.getPixels().data());

    /*m_sprite.setTexture(m_texture);*/
    /*m_sprite.setScale(static_cast<float>(m_scale), static_cast<float>(m_scale)); // Scale up*/
    m_window.draw(m_sprite);
    ImGui::SFML::Render(m_window);

    m_window.display();
}


void
Game::paintBrush()
{
    if (m_left_button_pressed) {
        sf::Vector2i pixelPos = sf::Mouse::getPosition(m_window);
        Vec2i simulationPos = pixelPos / m_scale;
        Vec2i previousSimulationPos = m_previousMousePos / m_scale;
        Action userAction = DrawLineAction{previousSimulationPos, simulationPos, m_drawLineWidth, m_drawPixelType};
        m_pixelGrid.userAction(userAction);
    }
}

void
Game::holdAndDragLine() 
{
    sf::Vector2i pixelPos = sf::Mouse::getPosition(m_window);
    Vec2i simulationPos = pixelPos / m_scale;
    if (!m_prev_left_button_pressed && m_left_button_pressed) { // the user first clicks the button
        m_dragLineStartSimulationPos = simulationPos; 
    } else if (m_prev_left_button_pressed && !m_left_button_pressed) {
        Action userAction = DrawLineAction{m_dragLineStartSimulationPos, simulationPos, m_drawLineWidth, m_drawPixelType};
        m_pixelGrid.userAction(userAction);
    }
}

void
Game::drawCircle()
{

}

void
Game::heldButtons() {
    sf::Vector2i pixelPos = sf::Mouse::getPosition(m_window);
    Vec2f pixelPos2f = Vec2f(pixelPos.x, pixelPos.y);
    if (m_leftClickAction == InputMode::PaintBrush) {
        paintBrush();   
    } else if (m_leftClickAction == InputMode::HoldAndDragLine) {
        holdAndDragLine();
    } else if (m_leftClickAction == InputMode::DrawCircle) {
        drawCircle();
    }

    m_prev_left_button_pressed = m_left_button_pressed;
    m_previousMousePos = pixelPos2f;
}


void 
Game::sUserInput() {
    sf::Event event;
    while (m_window.pollEvent(event)) {
        ImGui::SFML::ProcessEvent(m_window, event);

        if (event.type == sf::Event::Closed) {
            m_running = false;
        }

        if (event.type == sf::Event::MouseButtonPressed) {
            switch (event.mouseButton.button) {
                case sf::Mouse::Left:
                    m_left_button_pressed = true;
                case sf::Mouse::Right:
                    break;
                case sf::Mouse::Middle:
                    break;
                default:
                    break;
            }
        }

        if (event.type == sf::Event::MouseButtonReleased) {
            switch (event.mouseButton.button) {
                case sf::Mouse::Left:
                    m_left_button_pressed = false;
                case sf::Mouse::Right:
                    break;
                case sf::Mouse::Middle:
                    break;
                default:
                    break;
            }
        }
    }
}

void
Game::Gui() 
{
    ImGui::Begin("Pixel Simulator");

    if (ImGui::TreeNode("Draw Properties")) {

        ImGui::TreePop();
    }
}
