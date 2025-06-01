#include "Materials.h"
#include "Vec2.hpp"
#include "imgui-SFML.h"
#include <SFML/Config.hpp>
#include <bitset>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <random>
#include <strings.h>
#include <sys/types.h>
#include <unordered_map>
#include <variant>
#include <vector>
#include "BitOperationHelpers.hpp"

struct Pixel 
{
    uint8_t material;
};

struct SetPixelAction
{
    Vec2i pos;
    Pixel pixel_type;
};

struct DrawCircle
{
    Vec2i pos;
    float radius;
    Pixel pixel_type;
};

struct DrawLineAction
{
    Vec2i start;
    Vec2i end;
    float width;
    Pixel pixel_type;
};


using Action = std::variant<SetPixelAction, DrawCircle, DrawLineAction>;

// TODO test whether it is more efficient to store all the pixels in a single 
// 1D pixel and use maths to determine row and collum 
// need to test this to find out whether it is faster or not 
// using a 2D vector simplifies the program and is more intuitive
using SinglePixelGrid = std::vector<Pixel>;

// Custom game colors
const sf::Color GrassGreen(106, 190, 48);          // Lush grass
const sf::Color DirtBrown(155, 118, 83);           // Soil or dirt
const sf::Color WaterBlue(64, 164, 223, 200);      // Semi-transparent water
const sf::Color LavaOrange(255, 69, 0);            // Hot lava
const sf::Color SandYellow(237, 201, 175);         // Sandy ground
const sf::Color FireYellow(255, 200, 0, 220);      // Flickering fire (semi-transparent)
const sf::Color SmokeGray(96, 96, 96, 150);        // Translucent smoke
const sf::Color MetalGray(128, 128, 128);           // Metal surfaces

class PixelGrid
{
    SinglePixelGrid m_pixelGrid[2]; // double buffer of pixel grids
    std::vector<Vec2f> m_velocityField;
    int curr, next;
    int m_gridWidth;
    int m_gridHeight;
    std::vector<SetPixelAction> m_SetPixelQueue;
    std::vector<sf::Uint8> m_buffer;

    std::vector<Action> m_actionQueue;

    std::uniform_int_distribution<> m_2indexDistibution;

        // Create a random device and engine
    std::random_device rd; 
    std::mt19937 gen;

    // Define colors per material in RGBA, 4 bytes per material
    // Indexing: materialToColor_c[material * 4 + 0..3]
    std::vector<sf::Uint8> m_materialToColor_c = {
        // Air -> transparent (alpha 0)
        0, 0, 0, 0,

        // Sand -> sandy yellow
        194, 178, 128, 255,

        // Water -> blue, semi-transparent
        0, 0, 255, 180,

        // Stone -> grey
        128, 128, 128, 255
    };

    Vec2i positionFromIndex(size_t index) {
        if (index >= m_gridWidth * m_gridHeight) {
            throw std::out_of_range("Index out of bounds in positionFromIndex");
        }
        return Vec2i(index % m_gridWidth, index / m_gridWidth);
    }

    size_t indexFromPosition(Vec2i pos) {
        if (pos.x < 0 || pos.x >= m_gridWidth || pos.y < 0 || pos.y >= m_gridHeight) {
            throw std::out_of_range("Position out of bounds in indexFromPosition");
        }
        return m_gridWidth * pos.y + pos.x;
    }

    bool checkInBounds(Vec2i pos) {
        if (pos.x < 0 || pos.x >= m_gridWidth) {
            return false;
        }
        if (pos.y < 0 || pos.y >= m_gridHeight) {
            return false;
        }
        return true;
    }

    template<typename T>
    T selectRandomElement(std::vector<T> & vec) {
        int vectorLength = vec.size();
        auto theDistribution = std::uniform_int_distribution(0, vectorLength - 1);
        int theIndex = theDistribution(gen);
        return vec[theIndex];
    }

    int nextPositionPhysics(std::vector<std::vector<Vec2i>> nextPositionDeltasLevels, int index) 
    {
        Vec2i pos = positionFromIndex(index);
        int belowIndex;
        // deltas closer to the start have higher priority 
        Vec2i currentPosition = positionFromIndex(index);
        
        for (std::vector<Vec2i> nextPositionDeltas : nextPositionDeltasLevels) {
            std::vector<int> nextPositions = {};
            for (Vec2i deltaPos : nextPositionDeltas) {
                Vec2i trialPos = currentPosition + deltaPos;
                if (not checkInBounds(trialPos)) {
                    continue;
                }
                int trialIndex = indexFromPosition(trialPos);

                if (m_pixelGrid[curr][trialIndex].material == materials::air && 
                    m_pixelGrid[next][trialIndex].material == materials::air) {
                    if (nextPositionDeltas.size() == 1) {
                        return trialIndex;
                    }
                    nextPositions.push_back(trialIndex);
                }
            }
            if (nextPositions.empty() == false) {
                return selectRandomElement(nextPositions);
            }

        }

        return index;
    }

    bool hasProperty(uint8_t material, material_properties::PixelFlags property) {
        return bitop::flag_has_mask(material_properties::materialLookup[material].flags, property);
    }

    void doPhysics()
    {
        // clears all the pixels in the next pixel grid 
        // loops over the curret pixel grid placing pixels in the 
        // setting all the pixels in the new pixel grid 
        // then   exchanges curr and next setting the current pixel grid to
        // the previous iterations next pixel grid 

        // Clear the next grid first by resizing and filling with default Pixels (optional)
        clearNextPixelGrid();

        using nextPos  = std::vector<std::vector<Vec2i>>;
        nextPos nextPositionDeltasLevels_sand = {
            { Vec2i(0,1) }, 
            { Vec2i(-1,1), Vec2i(1,1) }
        };

        nextPos nextPositionDeltasLevels_water = {
            { Vec2i(0,-1) }, 
            { Vec2i(-1,-1), Vec2i(1,-1) },
            { Vec2i(-1,0), Vec2i(1,0) }
        };

        for (size_t i = 0; i < m_pixelGrid[curr].size(); ++i) {
            uint8_t currentMaterial = m_pixelGrid[curr][i].material;

            if (currentMaterial == materials::air) { continue; }
            /**/

            if (hasProperty(currentMaterial, material_properties::fallingPowder))
            {
                m_pixelGrid[next][nextPositionPhysics(nextPositionDeltasLevels_sand, i)] = m_pixelGrid[curr][i];               
            }
            
            else if(hasProperty(currentMaterial, material_properties::fixedSolid)) {
                m_pixelGrid[next][i] = m_pixelGrid[curr][i];
            }

            else if (hasProperty(currentMaterial, material_properties::fallingLiquid)) {
                m_pixelGrid[next][nextPositionPhysics(nextPositionDeltasLevels_water, i)] = m_pixelGrid[curr][i];               
            }
        }

    }

    void init()
    {
        initRandom();
        initGrid();
    }

    void initRandom()
    {
        gen = std::mt19937(rd());

        m_2indexDistibution = std::uniform_int_distribution<>(0,1);
    }

    void initGrid()
    {
        // TODO overload the function to take a config file name
        // and load all the pixels from that config file
        // currently this sets all the pixels to air 

        m_pixelGrid[curr].assign(m_gridWidth * m_gridHeight, Pixel{materials::air});
        clearNextPixelGrid();

        // drawing some pixels at the top of the screen
        for (int j = 0; j < m_gridHeight; j++) {
            for (int i = 0; i < m_gridWidth; i ++) {
                if (j > 9 && j < 11) {
                    m_pixelGrid[curr][j*m_gridWidth + i].material = materials::sand;
                }
            }
        }
    }

    void clearNextPixelGrid() {
        m_pixelGrid[next].assign(m_gridWidth * m_gridHeight, Pixel{materials::air});
    }

    void executeActions(std::vector<Action> & actions) 
    {
        /*std::cout << "Size of queue : " << actions.size() << "\n";*/
        for (auto a = actions.begin(); a != actions.end();) {
            if (std::holds_alternative<SetPixelAction>(*a)) {
                setPixel(std::get<SetPixelAction>(*a));
            } else if (std::holds_alternative<DrawLineAction>(*a)) {
                drawLine(std::get<DrawLineAction>(*a));
            } else if (std::holds_alternative<DrawCircle>(*a)) {
                drawCircle(std::get<DrawCircle>(*a));
            }
            a = actions.erase(a);
        }
    }

    void setPixel(SetPixelAction action) 
    {
        action.pos.print();
        m_pixelGrid[curr][indexFromPosition(action.pos)] = action.pixel_type;       
    }

    void drawLine(DrawLineAction action)
    {
        // Extract start and end points
        int x0 = action.start.x;
        int y0 = action.start.y;
        int x1 = action.end.x;
        int y1 = action.end.y;

        // Calculate differences
        int dx = std::abs(x1 - x0);
        int dy = std::abs(y1 - y0);

        // Determine the direction of the step
        int sx = (x0 < x1) ? 1 : -1;
        int sy = (y0 < y1) ? 1 : -1;

        int err = dx - dy;

        int x = x0;
        int y = y0;

        while (true) {
            // Set pixel at the current position
            setPixel(SetPixelAction{ Vec2i{x, y}, action.pixel_type });

            if (x == x1 && y == y1) break;

            int e2 = 2 * err;

            if (e2 > -dy) {
                err -= dy;
                x += sx;
            }

            if (e2 < dx) {
                err += dx;
                y += sy;
            }
        }
    }

    void drawCircle(DrawCircle action)
    {

    }

    void updateDrawBuffer()
    {
        m_buffer.assign(m_gridWidth * m_gridHeight * 4, 0);
        for (int j = 0; j < m_gridHeight; j++) 
        {
            for (int i = 0; i < m_gridWidth; i ++)
            {
                int q =  (j * m_gridWidth + i);
                int p = q * 4;
                m_buffer[p] = m_materialToColor_c[4 * m_pixelGrid[curr][q].material];
                m_buffer[p+1] = m_materialToColor_c[4 * m_pixelGrid[curr][q].material + 1];
                m_buffer[p+2] = m_materialToColor_c[4 * m_pixelGrid[curr][q].material + 2];
                m_buffer[p+3] = m_materialToColor_c[4 * m_pixelGrid[curr][q].material + 3];
            }
        }
    }

    void pingPongSwap() 
    {
        curr = 1 - curr;
        next = 1 - next;
    }

public:
    PixelGrid() : m_gridWidth(0), m_gridHeight(0) {}

    PixelGrid(int width, int height) 
        : m_gridWidth(width), m_gridHeight(height), curr(0), next(1),
        m_buffer(width * height * 4)
    {
        init();
    }

    void reset(int width, int height) 
    {
        m_gridWidth = width;
        m_gridHeight = height;
        m_buffer.resize(width * height * 4);

        init();
    }


    void update()
    {
        // loops over all pixels in the current pixel grid
        // executing any actions that are qued from either user input or any
        // other sources in the game system
        // then after this is finished call doPhysics()

        executeActions(m_actionQueue);
        doPhysics();
        updateDrawBuffer();

        /*std::cout << "done draw buffer\n";*/
        pingPongSwap();
    }

    void userAction(Action userAction) {
        m_actionQueue.push_back(userAction);
    }

    auto& getPixels() {
        /*for (int i = 0; i < 10; i++) {*/
        /*    for (int j = 0; j < 4 ; j ++) {*/
        /*        std::cout << static_cast<int>(m_buffers[curr][i*4 + j]) << " "  ;*/
        /*    };*/
        /*    std::cout <<  ", ";*/
        /*}*/
        /*std::cout << "\n";*/
        /*std::cout << m_pixelGrid[curr].size() << "\n";*/
        return m_buffer;
    }
};
