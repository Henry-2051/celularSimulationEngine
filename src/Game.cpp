#include <SFML/Graphics/Texture.hpp>
#include <iostream>
#include <string>
#include <sys/types.h>
#include "Game.h"
#include "Materials.h"
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

    m_window.create(sf::VideoMode(static_cast<u_int>(m_windowWidth), static_cast<uint>(m_windowHeight)),
                    "Falling sand sim");
    m_window.setFramerateLimit(static_cast<u_int>(m_fps));

    ImGui::SFML::Init(m_window);

    ImGui::GetStyle().ScaleAllSizes(2.0f);
    ImGui::GetIO().FontGlobalScale = 2.0f;
    /*m_pixelGrid = PixelGrid(m_windowWidth / m_scale, m_windowHeight / m_scale);*/

    m_texture.create(static_cast<u_int>(m_width), static_cast<u_int>(m_height));

    /*std::cout << "Making sprite\n";*/
    // Create a sprite to display the texture
    m_sprite.setTexture(m_texture);
    m_sprite.setScale(static_cast<float>(m_scale), static_cast<float>(m_scale)); // Scale up
    /*std::cout << "sprite made\n";*/

    previousTime = clock::now();
}

void
Game::run() 
{
    while (m_running) 
    {
        m_frameCount +=1 ;
        auto currentTime = clock::now();

        seconds elapsed = currentTime - previousTime;
        if (elapsed.count() > 1.0) {
            std::cout << "FPS: " << (m_frameCount) / elapsed.count() << std::endl; 
            m_frameCount = 0;
            previousTime = currentTime;
        }
        /*std::cout << "Running program\n";*/
        m_pixelGrid.update();       
        /*std::cout << "updated\n";*/
        ImGui::SFML::Update(m_window, m_deltaClock.restart());
        sUserInput();
        heldButtons();
        sGui();
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
        Vec2st simulationPos = Vec2i{pixelPos / static_cast<int>(m_scale)};
        Vec2i previousSimulationPos = m_previousMousePos / m_scale;
        Action userAction = DrawLineAction{previousSimulationPos, simulationPos, m_paintBrushWidth-1, m_drawPixelType};
        m_pixelGrid.userAction(userAction);
    }
}

void
Game::tryIgnite()
{
    if (m_left_button_pressed) {
        sf::Vector2i pixelPos = sf::Mouse::getPosition(m_window);
        Vec2st simulationPos = Vec2i{pixelPos / static_cast<int>(m_scale)};
        Action userAction = IgnitionAction{simulationPos};
        m_pixelGrid.userAction(userAction);
    }
}
void
Game::holdAndDragLine() 
{
    sf::Vector2i pixelPos = sf::Mouse::getPosition(m_window);
    Vec2st simulationPos = Vec2i{pixelPos / static_cast<int>(m_scale)};
    if (hasUserLeftClicked()) {
        m_dragLineStartSimulationPos = simulationPos; 
    } else if (hasUserLeftReleased()) {
        Action userAction = DrawLineAction{m_dragLineStartSimulationPos, simulationPos, m_drawLineWidth, m_drawPixelType};
        m_pixelGrid.userAction(userAction);
    }
}

bool
Game::hasUserLeftClicked()
{
    return !m_prev_left_button_pressed && m_left_button_pressed;
}

bool
Game::hasUserLeftReleased()
{
    return m_prev_left_button_pressed && !m_left_button_pressed ;
}

void
Game::drawCircle()
{
    if (hasUserLeftClicked()) 
    {
        sf::Vector2i pixelPos = sf::Mouse::getPosition(m_window);
        Vec2st simulationPos = Vec2i{pixelPos / static_cast<int>(m_scale)};
        Action userAction = DrawCircle{simulationPos, m_drawCircleRadius, m_drawPixelType};
        m_pixelGrid.userAction(userAction);
    }
}

void
Game::drawParallelogram()
{
    sf::Vector2i pixelPos = sf::Mouse::getPosition(m_window);
    Vec2st simulationPos = Vec2i{pixelPos / static_cast<int>(m_scale)};

    if (hasUserLeftClicked()) {
        if (m_currentParallelogramState == WaitingForStart) {
            m_firstParallelogramPoint = simulationPos;
            m_currentParallelogramState = HeldDown1;
        } 
        else if (m_currentParallelogramState == WaitingForSecondClick) {
            m_thirdParallelogramPoint = simulationPos;
            m_currentParallelogramState = DrawingShape;
        }
    } 
    else if (hasUserLeftReleased()) {
        if (m_currentParallelogramState == HeldDown1) {
            m_secondParallelogramPoint = simulationPos;
            m_currentParallelogramState = WaitingForSecondClick;
        }
    }

    if (m_currentParallelogramState == DrawingShape) {
        m_currentParallelogramState = WaitingForStart;
        Action action = DrawParallelogramAction{m_firstParallelogramPoint, m_secondParallelogramPoint, m_thirdParallelogramPoint, m_drawPixelType};
        m_pixelGrid.userAction(action);
    }
}

void
Game::heldButtons() {
    sf::Vector2i pixelPos = sf::Mouse::getPosition(m_window);
    bool isHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
    Vec2i pixelPos2i = Vec2i(pixelPos.x, pixelPos.y);
    if (!isHovered) {
        if (m_drawMethod == inputModes::paintBrush) {
            paintBrush();   
        } else if (m_drawMethod== inputModes::holdAndDragLine) {
            holdAndDragLine();
        } else if (m_drawMethod== inputModes::drawCircle) {
            drawCircle();
        } else if (m_drawMethod == inputModes::drawParallelogram) {
            drawParallelogram();
        } else if (m_drawMethod == inputModes::ignitePixel) {
            tryIgnite();
        }

    }

    m_prev_left_button_pressed = m_left_button_pressed;
    m_previousMousePos = pixelPos2i;
}


void 
Game::sUserInput() {
    sf::Event event;
    if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) {
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
}

void
Game::sGui() 
{
    ImGui::Begin("Pixel Simulator");

    if (ImGui::TreeNode("Material selection")) {
        if (ImGui::BeginListBox("")) {
            static size_t itemSelectedIDX = 1;
            int itemHighlightedIDX = -1;
            for (size_t i = 0; i < sizeof(materials::materialNames) / sizeof(materials::materialNames[0]); i++) {
                const bool isSlected = (itemSelectedIDX == i);
                if (ImGui::Selectable(materials::materialNames[i], isSlected)) {
                    itemSelectedIDX = i;
                }
                if (ImGui::IsItemHovered()) {
                    itemHighlightedIDX = static_cast<int>(i);
                }
                if (isSlected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndListBox();

            m_drawPixelType.material = materials::materials[itemSelectedIDX];
        }
        ImGui::TreePop();
        }
    if (ImGui::TreeNode("Draw method selection")) {
        if (ImGui::BeginListBox("")) {
            static size_t drawingMethodSelectedIDX = 0;
            int drawingMethodHighlightedIDX = -1;
            for (size_t i = 0; i < sizeof(inputModes::inputModeNames) / sizeof(inputModes::inputModeNames[0]); i++) {
                const bool isSelected = (i == drawingMethodSelectedIDX);
                if (ImGui::Selectable(inputModes::inputModeNames[i], isSelected)) {
                    drawingMethodSelectedIDX = i;
                }
                if (ImGui::IsItemHovered()) {
                    drawingMethodHighlightedIDX = static_cast<int>(i);
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }

            m_drawMethod = inputModes::inputModes[drawingMethodSelectedIDX];
            ImGui::EndListBox();
        }
        
        if (m_drawMethod == inputModes::paintBrush) 
        {
            float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Paint brush size:");
            ImGui::SameLine();
            ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
            if (ImGui::ArrowButton("##left", ImGuiDir_Left) && m_paintBrushWidth > 1) { m_paintBrushWidth--; }
            ImGui::SameLine(0.0f, spacing);
            if (ImGui::ArrowButton("##right", ImGuiDir_Right)) { m_paintBrushWidth++; }
            ImGui::PopItemFlag();
            ImGui::SameLine();
            ImGui::Text("%d", m_paintBrushWidth);
        } 
        else if (m_drawMethod == inputModes::holdAndDragLine) 
        {
            float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Draw line width:");
            ImGui::SameLine();
            ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
            if (ImGui::ArrowButton("##left", ImGuiDir_Left) && m_drawLineWidth > 1) { m_drawLineWidth--; }
            ImGui::SameLine(0.0f, spacing);
            if (ImGui::ArrowButton("##right", ImGuiDir_Right)) { m_drawLineWidth++; }
            ImGui::PopItemFlag();
            ImGui::SameLine();
            ImGui::Text("%d", m_drawLineWidth);
        } 
        else if (m_drawMethod == inputModes::drawCircle) 
        {
            float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Circle radius:");
            ImGui::SameLine();
            ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
            if (ImGui::ArrowButton("##left", ImGuiDir_Left) && m_drawCircleRadius> 1) { m_drawCircleRadius--; }
            ImGui::SameLine(0.0f, spacing);
            if (ImGui::ArrowButton("##right", ImGuiDir_Right)) { m_drawCircleRadius++; }
            ImGui::PopItemFlag();
            ImGui::SameLine();
            ImGui::Text("%d", m_drawCircleRadius);
        } 
        else if (m_drawMethod == inputModes::drawParallelogram) 
        {
        }
        else if (m_drawMethod == inputModes::ignitePixel) 
        {
        }

        ImGui::TreePop();
    } 



    ImGui::End();
}
