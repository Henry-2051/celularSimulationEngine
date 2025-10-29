#include <SFML/Graphics/Texture.hpp>
#include <iostream>
#include <string>
#include <sstream>
#include <sys/types.h>
#include "Game.h"
#include "Materials.h"
#include "UserInputOptions.h"
#include "Vec2.hpp"
#include <fstream>
#include "imgui-SFML.h"
#include <algorithm>
#include <cctype>
#include "imgui.h"
#include <map>
#include <variant>
#include <format>
#include <charconv>
#include <type_traits>
#include <functional>


Game::Game(const std::string& config)
{
    auto options = load_config(config);
    init(options);
}

// 0: int, 1: size_t, 2: bool, 3: string
LoadedConfig Game::load_config(std::string filePath) {
    LoadedConfig config {};

    std::map<std::string, std::function<void(LoadedConfig&, const std::string&)>> setter_register =  {
        {"renderingEngine", make_setter(&LoadedConfig::rendering_engine)},
        {"gridWidth", make_setter(&LoadedConfig::pGrid_width)},
        {"gridHeight", make_setter(&LoadedConfig::pGrid_height)},
        {"fps", make_setter(&LoadedConfig::fps)},
        {"scale", make_setter(&LoadedConfig::scale)},
        {"windowWidth", make_setter(&LoadedConfig::window_width)},
        {"windowHeight", make_setter(&LoadedConfig::window_height)},
    };

    std::ifstream configFile {filePath} ;
    if (!configFile.is_open()) {
        std::cerr << "Error: cannot open " << filePath << "\n";
    };
    

    std::string line;
    while (std::getline(configFile, line)) {
        if (line.empty() || line[0] == '#') { continue; }

        std::istringstream ss{line};

        std::string conf_name, conf_value ;

        if (!(ss >> conf_name >> conf_value)) {
            std::cerr << std::format("mallformed conf line with less than two elements: {}\n", line);
        };


        auto register_pair_it = setter_register.find(conf_name);

        if (register_pair_it == setter_register.end()) {
            std::cerr << std::format("unknown config key {}\n", conf_name);
        }

        (register_pair_it->second)(config, conf_value);
    }
    printConfig(config, std::cout);

    return config; 
};

void
Game::initializeSFML2(LoadedConfig & l_config) {
    SFML2State & sstate = state.sfml2_state;
    LoadedConfig & pstate = state.persistent_state;

    sstate.window_height = pstate.scale * pstate.pGrid_height;
    sstate.window_width  = pstate.scale * pstate.pGrid_width;

    sstate.draw_texture_height = sstate.window_height / pstate.scale;
    sstate.draw_texture_width = sstate.window_width / pstate.scale;
    
    sstate.window.create(
        sf::VideoMode(static_cast<u_int>(state.sfml2_state.window_width), 
                      static_cast<uint>(state.sfml2_state.window_height)),
        "Falling sand sim");

    state.sfml2_state.window.setFramerateLimit(state.persistent_state.fps);

    ImGui::SFML::Init(state.sfml2_state.window);

    ImGui::GetStyle().ScaleAllSizes(1.0f);
    ImGui::GetIO().FontGlobalScale = 2.0f;

    sstate.texture.create(
        static_cast<u_int>(sstate.draw_texture_width), static_cast<u_int>(sstate.draw_texture_height)
    );
    sstate.sprite.setTexture(sstate.texture);
    sstate.sprite.setScale(static_cast<float>(pstate.scale), static_cast<float>(pstate.scale));
}

void
Game::init(LoadedConfig l_config)
{
    state.persistent_state = l_config;
    state.draw_pixel_type = {materials::sand, 0, material_properties::IsPowder};
    state.pixel_grid = PixelGrid(l_config.pGrid_width, l_config.pGrid_height);
    state.parallelogramState = ParallelogramState();

    initializeSFML2(l_config);

    previousTime = clock::now();
}

void
Game::run() 
{
    while (state.running) 
    {
        state.frame_count +=1 ;
        auto currentTime = clock::now();

        seconds elapsed = currentTime - previousTime;
        if (elapsed.count() > 1.0) {
            std::cout << "FPS: " << (state.frame_count) / elapsed.count() << std::endl; 
            state.frame_count = 0;
            previousTime = currentTime;
        }
        /*std::cout << "Running program\n";*/
        state.pixel_grid.update();       
        /*std::cout << "updated\n";*/
        ImGui::SFML::Update(state.sfml2_state.window, state.sfml2_state.delta_clock.restart());

        sUserInput();
        heldButtons();
        sGui();
        sRender();
    }
}

void
Game::sRender()
{
    state.sfml2_state.window.clear();
    /*std::cout << "in rendering function\n";*/

    state.sfml2_state.texture.update(state.pixel_grid.getPixels().data());

    /*m_sprite.setTexture(m_texture);*/
    /*m_sprite.setScale(static_cast<float>(m_scale), static_cast<float>(m_scale)); // Scale up*/
    state.sfml2_state.window.draw(state.sfml2_state.sprite);
    ImGui::SFML::Render(state.sfml2_state.window);

    state.sfml2_state.window.display();
}


void
Game::paintBrush()
{
    if (state.left_button_pressed) {
        sf::Vector2i pixelPos = sf::Mouse::getPosition(state.sfml2_state.window);
        Vec2i simulationPos = Vec2i{pixelPos / static_cast<int>(state.persistent_state.scale)};
        Vec2i previousSimulationPos = state.previous_mouse_position/ state.persistent_state.scale;
        ActionIncludingPair userAction = 
        DrawLineAction{previousSimulationPos, simulationPos, state.paint_brush_width-1, state.draw_pixel_type};
        state.pixel_grid.userAction(userAction);
    }
}

void
Game::tryIgnite()
{
    if (state.left_button_pressed) {
        sf::Vector2i pixelPos = sf::Mouse::getPosition(state.sfml2_state.window);
        Vec2i simulationPos = Vec2i{pixelPos / static_cast<int>(state.persistent_state.scale)};
        ActionIncludingPair userAction = IgnitionAction{simulationPos};
        state.pixel_grid.userAction(userAction);
    }
}
void Game::holdAndDragLine()
{
    sf::Vector2i pixelPos = sf::Mouse::getPosition(state.sfml2_state.window);
    Vec2i simulationPos = Vec2i{pixelPos / static_cast<int>(state.persistent_state.scale)};
    if (hasUserLeftClicked()) {
        state.drag_line_start_simulation_pos = simulationPos; 
    } else if (hasUserLeftReleased()) {
        ActionIncludingPair userAction = 
        DrawLineAction{state.drag_line_start_simulation_pos, simulationPos, state.draw_line_width, state.draw_pixel_type};
        state.pixel_grid.userAction(userAction);
    }
}

bool
Game::hasUserLeftClicked()
{
    return !state.prev_left_button_pressed && state.left_button_pressed;
}

bool
Game::hasUserLeftReleased()
{
    return state.prev_left_button_pressed && !state.left_button_pressed;
}

void
Game::drawCircle()
{
    if (hasUserLeftClicked()) 
    {
        sf::Vector2i pixelPos = sf::Mouse::getPosition(state.sfml2_state.window);
        Vec2i simulationPos = Vec2i{pixelPos / static_cast<int>(state.persistent_state.scale)};
        ActionIncludingPair userAction = 
        DrawCircle{simulationPos, state.draw_circle_radius, state.draw_pixel_type};

        state.pixel_grid.userAction(userAction);
    }
}

void
Game::drawParallelogram()
{
    sf::Vector2i pixelPos = sf::Mouse::getPosition(state.sfml2_state.window);
    Vec2i simulationPos = Vec2i{pixelPos / static_cast<int>(state.persistent_state.scale)};

    ParallelogramState & astate = state.parallelogramState;

    if (hasUserLeftClicked()) {

        if (astate.current_parallelogram_state == astate.WaitingForStart) {
            std::cout << "here1\n";
            astate.first_parallelogram_point = simulationPos;
            astate.current_parallelogram_state = astate.HeldDown;
        } 
        else if (astate.current_parallelogram_state == astate.WaitingForSecondClick) {
            std::cout << "here3\n";
            astate.third_parallelogram_point = simulationPos;

            astate.current_parallelogram_state = astate.WaitingForStart;
            ActionIncludingPair action = 
            DrawParallelogramAction{astate.first_parallelogram_point, astate.second_parallelogram_point, 
                astate.third_parallelogram_point, state.draw_pixel_type};

            state.pixel_grid.userAction(action);
        }
    } 
    else if (hasUserLeftReleased()) {
            std::cout << "here2\n";

        if (astate.current_parallelogram_state == astate.HeldDown) {
            std::cout << "here2.5\n";

            astate.second_parallelogram_point = simulationPos;
            astate.current_parallelogram_state = astate.WaitingForSecondClick;
        }
    }
}

void
Game::heldButtons() {
    sf::Vector2i pixelPos = sf::Mouse::getPosition(state.sfml2_state.window);
    bool isHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
    Vec2i pixelPos2i = Vec2i(pixelPos.x, pixelPos.y);
    if (!isHovered) {
        if (state.draw_method == inputModes::paintBrush) {
            paintBrush();   
        } else if (state.draw_method == inputModes::holdAndDragLine) {
            holdAndDragLine();
        } else if (state.draw_method == inputModes::drawCircle) {
            drawCircle();
        } else if (state.draw_method == inputModes::drawParallelogram) {
            drawParallelogram();
        } else if (state.draw_method == inputModes::ignitePixel) {
            tryIgnite();
        }

    }

    state.prev_left_button_pressed = state.left_button_pressed;
    state.previous_mouse_position= pixelPos2i;
}


void 
Game::sUserInput() {
    sf::Event event;
    if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) {
        while (state.sfml2_state.window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(state.sfml2_state.window, event);

            if (event.type == sf::Event::Closed) {
                state.running= false;
            }

            if (event.type == sf::Event::MouseButtonPressed) {
                switch (event.mouseButton.button) {
                    case sf::Mouse::Left:
                        state.left_button_pressed = true;
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
                        state.left_button_pressed = false;
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

            state.draw_pixel_type.material = materials::materials[itemSelectedIDX];
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

            state.draw_method= inputModes::inputModes[drawingMethodSelectedIDX];
            ImGui::EndListBox();
        }
        
        if (state.draw_method == inputModes::paintBrush) 
        {
            float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Paint brush size:");
            ImGui::SameLine();
            ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
            if (ImGui::ArrowButton("##left", ImGuiDir_Left) && state.paint_brush_width > 1) { state.paint_brush_width --; }
            ImGui::SameLine(0.0f, spacing);
            if (ImGui::ArrowButton("##right", ImGuiDir_Right)) { state.paint_brush_width ++; }
            ImGui::PopItemFlag();
            ImGui::SameLine();
            ImGui::Text("%d", state.paint_brush_width);
        } 
        else if (state.draw_method == inputModes::holdAndDragLine) 
        {
            float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Draw line width:");
            ImGui::SameLine();
            ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
            if (ImGui::ArrowButton("##left", ImGuiDir_Left) && state.draw_line_width> 1) { state.draw_line_width--; }
            ImGui::SameLine(0.0f, spacing);
            if (ImGui::ArrowButton("##right", ImGuiDir_Right)) { state.draw_line_width++; }
            ImGui::PopItemFlag();
            ImGui::SameLine();
            ImGui::Text("%d", state.draw_line_width);
        } 
        else if (state.draw_method == inputModes::drawCircle) 
        {
            float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Circle radius:");
            ImGui::SameLine();
            ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
            if (ImGui::ArrowButton("##left", ImGuiDir_Left) && state.draw_circle_radius > 1) { state.draw_circle_radius --; }
            ImGui::SameLine(0.0f, spacing);
            if (ImGui::ArrowButton("##right", ImGuiDir_Right)) { state.draw_circle_radius ++; }
            ImGui::PopItemFlag();
            ImGui::SameLine();
            ImGui::Text("%d", state.draw_circle_radius );
        } 
        else if (state.draw_method == inputModes::drawParallelogram) 
        {
        }
        else if (state.draw_method == inputModes::ignitePixel) 
        {
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Window movement")) {
        ImGui::DragInt2("Window position", (int*)&state.window_position);

        float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
        ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
        if (ImGui::ArrowButton("##left", ImGuiDir_Left)) { state.persistent_state.scale--; }
        ImGui::SameLine(0.0f, spacing);
        if (ImGui::ArrowButton("##right", ImGuiDir_Right)) { state.persistent_state.scale++; }
        ImGui::PopItemFlag();
        ImGui::SameLine();
        ImGui::Text("%d", state.persistent_state.scale);
        ImGui::TreePop();
    }
    ImGui::End();
}
