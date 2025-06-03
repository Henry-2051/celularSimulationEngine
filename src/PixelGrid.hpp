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

        auto f_checkPos = [&currentPosition, this](Vec2i movement) {
            return (safeCheckIsAir(currentPosition + movement));
        };

        if (randomRight) /* gen belongs to the object this method belong to*/ {
            if (f_checkPos(right)) {
                if (f_checkPos(right2)) { return maybeResult(currentPosition + right2);}
                return maybeResult(currentPosition + right);
            }
        } else {
            if (f_checkPos(left)) {
                if (f_checkPos(left2)) { return maybeResult(currentPosition + left2); }
                return maybeResult(currentPosition + left);
            }
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
        return (m_pixelGrid[curr][indexFromPosition(pos)].material == materials::air &&
                m_pixelGrid[next][indexFromPosition(pos)].material == materials::air);
    };

    bool safeCheckIsAir(Vec2i pos) const {
        if (isInBounds(pos)) {
            return checkIsAir(pos);
        } else {return false;};
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


        // the pixel must have a path to move through if it moves diagonally
        // othersie it seemingly phases through containers


        for (size_t i = 0; i < m_pixelGrid[curr].size(); ++i) {
            uint8_t currentMaterial = m_pixelGrid[curr][i].material;

            Vec2i currentPos = positionFromIndex(i);
            maybeResult<Vec2i> nextPos;

            auto scopeCapturedSandPhysics = [&currentPos, this](){return this->generalSandPhysics(currentPos);};

            auto scopeCapturedWaterPhysics = [&currentPos, this](){return this->generalWaterPhysics(currentPos, m_2indexDistibution(gen) == 1);};

            auto scopeCaptureFallingPhysics = [this](Vec2i cPos){return this->generalFallingPhysics(cPos); };

            auto waterThenFallingPhysics = [&scopeCaptureFallingPhysics, &scopeCapturedWaterPhysics](){ 
                return (
                    scopeCapturedWaterPhysics()
                    .passThrough(scopeCaptureFallingPhysics)
            );};

            if (currentMaterial == materials::air) { continue; }

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
                int nextIndex = indexFromPosition(nextPos.getValue());
                m_pixelGrid[next][nextIndex] = m_pixelGrid[curr][i];
            } else {
                m_pixelGrid[next][i] = m_pixelGrid[curr][i];
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
