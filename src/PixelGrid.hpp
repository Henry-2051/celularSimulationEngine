#include "Materials.h"
#include "Vec2.hpp"
#include "imgui-SFML.h"
#include <SFML/Config.hpp>
#include <algorithm>
#include <bitset>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <ostream>
#include <random>
#include <stdexcept>
#include <strings.h>
#include <sys/types.h>
#include <unordered_map>
#include <variant>
#include <vector>
#include "BitOperationHelpers.hpp"

template<typename T>
class maybeResult{
    bool m_isNothing;
    T m_value;

public:
    maybeResult() : m_isNothing(true) {};

    maybeResult(T val) : m_isNothing(false), m_value(val) {};

    maybeResult<T>
    setValue(T val) {
        m_value = val;
        m_isNothing = false;
        return *this;
    }

    maybeResult<T> merge(const maybeResult<T>& tryUntilOther) const {
        return (!m_isNothing ? *this : tryUntilOther);
    }

    // Higher-order method: takes a function generating tryUntilResult<T>, merges it with current
    template<typename Func>
    maybeResult<T> tryWith(Func f) const {
        maybeResult<T> other = f();
        return this->merge(other);
    }

    maybeResult<T> operator + (const maybeResult<T> m_val2) {
        if (this->exists() && m_val2.exists()) {
            return maybeResult(m_value + m_val2.m_value);
        } else { return maybeResult<T>(); }
    }

    template<typename Func>
    maybeResult<T> passThrough(Func f) {
        if (this->exists()) {
            maybeResult<T> result_val= f(m_value);
            return (result_val.exists() ? result_val : *this);
        } else {
            return *this;
        }
    }

    template<typename Func>
    maybeResult<T> nothingThen(Func f) {
        if (m_isNothing) {
            return f();
        } else {
            return *this;
        }
    }

    void print() {
        if (m_isNothing) {
            std::cout << "Nothing" << std::endl;
        } else {
            std::cout << "Value: "  << m_value << std::endl;
        }
    }

    T getValue() {
        if (not m_isNothing) {
            return m_value;
        } else {
            throw std::runtime_error("Trying to get the value of Nothing\n");
        }
    }
    bool exists() {
        return not m_isNothing;
    }
};

struct Pixel 
{
    uint8_t material;
    uint8_t updateFrame;
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
using SinglePixelGrid = std::vector<std::vector<Pixel>>;

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
    SinglePixelGrid m_pixelGrid; // double buffer of pixel grids
    std::vector<std::vector<Vec2f>> m_velocityField;
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

    uint8_t m_globalUpdateFrame = 0;

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

    Vec2i positionFromIndex(size_t index) const {
        if (index >= m_gridWidth * m_gridHeight) {
            throw std::out_of_range("Index out of bounds in positionFromIndex");
        }
        return Vec2i(index % m_gridWidth, index / m_gridWidth);
    }

    size_t indexFromPosition(Vec2i pos) const {
        if (pos.x < 0 || pos.x >= m_gridWidth || pos.y < 0 || pos.y >= m_gridHeight) {
            throw std::out_of_range("Position out of bounds in indexFromPosition");
        }
        return m_gridWidth * pos.y + pos.x;
    }

    bool isInBounds(Vec2i pos) const {
        if (pos.x < 0 || pos.x >= m_gridWidth) {
            /*pos.print();*/
            return false;
        }
        if (pos.y < 0 || pos.y >= m_gridHeight) {
            /*pos.print();*/
            return false;
        }
        return true;
    }

    template<typename T>
    T selectRandomElement(std::vector<T> & vec) {
        int vectorLength = vec.size();
        if (vectorLength == 1) {
            return vec[0];
        }
        auto theDistribution = std::uniform_int_distribution(0, vectorLength - 1);
        int theIndex = theDistribution(gen);
        return vec[theIndex];
    }

    maybeResult<Vec2i> generalSandPhysics(Vec2i currentPosition) const {
        const Vec2i down = Vec2i(0,1);
        const Vec2i downRight = Vec2i(1,1);
        const Vec2i downLeft = Vec2i(-1,1);
        const Vec2i right = Vec2i(1,0);
        const Vec2i left = Vec2i(-1,0);

        auto f_checkPos = [&currentPosition, this](Vec2i movement) {
            return (safeCheckIsAir(currentPosition + movement));
        };

        if (f_checkPos(down)) 
        {
            return maybeResult(currentPosition + down);
        } 
        else if (f_checkPos(right) && f_checkPos(downRight)) 
        {
            return maybeResult(currentPosition + downRight);
        } 
        else if (f_checkPos(left) && f_checkPos(downLeft)) 
        {
            return maybeResult(currentPosition + downLeft);
        } 
        else 
        {
            return maybeResult<Vec2i>();
        }
    }

    maybeResult<Vec2i> generalWaterPhysics(Vec2i currentPosition, bool randomRight) const {
        const Vec2i right = Vec2i(1,0);
        const Vec2i right2 = Vec2i(2,0);
        const Vec2i left = Vec2i(-1,0);
        const Vec2i left2 = Vec2i(-2,0);
        const Vec2i down = Vec2i(0,1);

        auto f_checkPos = [&currentPosition, this](Vec2i movement) {
            return (safeCheckIsAir(currentPosition + movement));
        };

        bool rightClear = f_checkPos(right);
        bool leftClear = f_checkPos(left);
        bool left2clear = f_checkPos(left2);
        bool right2Clear = f_checkPos(right2);

        if (rightClear && leftClear) {
            if (right2Clear && left2clear) {
                return maybeResult(currentPosition + (randomRight ? left : right));
            } else if (right2Clear) {
                return maybeResult(currentPosition + right2);
            } else if (left2clear) {
                return maybeResult(currentPosition + left2);
            }
            return maybeResult(currentPosition + (randomRight ? left : right));
        } else if (rightClear) {
            if (right2Clear) {
                return maybeResult(currentPosition + right2);
            }
            return maybeResult(currentPosition + right);
        } else if (leftClear) {
            if (left2clear) {
                return maybeResult(currentPosition + left2);
            }
            return maybeResult(currentPosition + left);
        }

        return maybeResult<Vec2i>();
    }

    maybeResult<Vec2i> generalFallingPhysics(Vec2i currentPosition) {
        const Vec2i down = Vec2i(0,1);

        auto f_checkPos = [&currentPosition, this](Vec2i movement) {
            return (safeCheckIsAir(currentPosition + movement));
        };

        if (f_checkPos(down)) {
            return maybeResult(currentPosition + down);
        }
        else { return maybeResult<Vec2i>(); }
    }

    bool checkIsAir(Vec2i pos) const {
        return (m_pixelGrid[pos.x][pos.y].material == materials::air);
    };

    bool safeCheckIsAir(Vec2i pos) const {
        if (isInBounds(pos)) {
            return checkIsAir(pos);
        } else {return false;};
    }


    bool hasProperty(uint8_t material, material_properties::PixelFlags property) {
        return bitop::flag_has_mask(material_properties::materialLookup[material].flags, property);
    }

    void doPhysics() {
        m_globalUpdateFrame ++;
        bool n2 = bitop::check_nth_bit(m_globalUpdateFrame, 1);
        if (bitop::check_nth_bit(m_globalUpdateFrame, 0)) {
            for (int xi = 0; xi < m_gridWidth; xi++) {
                if (n2) {
                    for (int yi = 0; yi < m_gridHeight; yi ++) {
                        doPhysicsOnPixel(xi,yi);
                    }
                } else {
                    for (int yi = m_gridHeight - 1; yi !=0; yi --) {
                        doPhysicsOnPixel(xi,yi);
                    }
                }
            }
        } else {
            for (int xi = m_gridWidth - 1; xi !=0; xi --) {
                if (n2) {
                    for (int yi = 0; yi < m_gridHeight; yi ++) {
                        doPhysicsOnPixel(xi,yi);
                    }
                } else {
                    for (int yi = m_gridHeight - 1; yi !=0; yi --) {
                        doPhysicsOnPixel(xi,yi);
                    }
                }
            }
        }
    }

    void doPhysicsOnPixel(int col, int row)
    {
        // the pixel must have a path to move through if it moves diagonally
        // othersie it seemingly phases through containers

        // assumption in all our next position functions, they cant returrn the same position they were given otherwise
        // that pixel would just be set to air, the maybe type is to represent the case that the function does not find
        // a new place to put the pixel

        uint8_t currentMaterial = m_pixelGrid[col][row].material;

        if (currentMaterial == materials::air) { return; }

        if (m_pixelGrid[col][row].updateFrame == m_globalUpdateFrame) { return; };

        Vec2i currentPos = Vec2i(col, row);
        maybeResult<Vec2i> nextPos;

        auto scopeCapturedSandPhysics = [&currentPos, this](){return this->generalSandPhysics(currentPos);};
        bool randomBool = m_2indexDistibution(gen);

        auto scopeCapturedWaterPhysics = [&currentPos, this, randomBool](){return this->generalWaterPhysics(currentPos, randomBool);};

        auto scopeCaptureFallingPhysics = [this](Vec2i cPos){return this->generalFallingPhysics(cPos); };


        if (hasProperty(currentMaterial, material_properties::fallingLiquid))
        {
            nextPos = maybeResult<Vec2i>()
                .tryWith(scopeCapturedSandPhysics)
                .tryWith(scopeCapturedWaterPhysics);
        }
        
        else if(hasProperty(currentMaterial, material_properties::fallingPowder)) {
            nextPos = maybeResult<Vec2i>()
                .tryWith(scopeCapturedSandPhysics);
        }

        else if (hasProperty(currentMaterial, material_properties::fixedSolid)) {
            nextPos = maybeResult<Vec2i>(); // essentially a nothing
        }

        else {
            throw std::runtime_error("UnknownMaterial fix in doPhysics method of pixelgrid class in PixelGridd.hpp");
        } 

        if (nextPos.exists()) {
            Vec2i v_nextPos = nextPos.getValue();
            m_pixelGrid[v_nextPos.x][v_nextPos.y] = m_pixelGrid[col][row];
            m_pixelGrid[v_nextPos.x][v_nextPos.y].updateFrame = m_globalUpdateFrame;
            m_pixelGrid[col][row].material = materials::air;
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
        m_pixelGrid.assign(m_gridWidth, std::vector<Pixel>(m_gridHeight, Pixel{materials::air}));
        // drawing some pixels at the top of the screen
        /*for (int i = 0; i < m_gridWidth; i++) {*/
        /*    for (int j = 0; j < m_gridHeight; j ++) {*/
        /*        if (j > 9 && j < 11) {*/
        /*            m_pixelGrid[i][j].material = materials::sand;*/
        /*        }*/
        /*    }*/
        /*}*/
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
        m_pixelGrid[action.pos.x][action.pos.y] = action.pixel_type;       
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
        for (int i = 0; i < m_gridWidth; i++) 
        {
            for (int j = 0; j < m_gridHeight; j ++)
            {
                int q =  (j * m_gridWidth + i);
                int p = q * 4;
                m_buffer[p  ] = m_materialToColor_c[4 * m_pixelGrid[i][j].material];
                m_buffer[p+1] = m_materialToColor_c[4 * m_pixelGrid[i][j].material + 1];
                m_buffer[p+2] = m_materialToColor_c[4 * m_pixelGrid[i][j].material + 2];
                m_buffer[p+3] = m_materialToColor_c[4 * m_pixelGrid[i][j].material + 3];
            }
        }
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
