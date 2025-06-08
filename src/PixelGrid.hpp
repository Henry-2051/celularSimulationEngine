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
#include "maybeResult.hpp"


struct Pixel 
{
    uint8_t material;
    uint8_t updateFrame;
    uint8_t properties;
};

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

// struct FirePixel
// {
//     uint16_t remainingBurnTime;
//     uint8_t  percentageSpreadChance;
//     bool     requiresAirToSpread;
// };
using FirePixel = material_properties::ignitionProperties;

struct m_IgniteAction
{
    FirePixel fireProperties;
    Vec2i     pos;
};

struct IgnitionAction {
    Vec2i pos;
};

using Action = std::variant<SetPixelAction, DrawCircle, DrawLineAction, DrawParallelogramAction, IgnitionAction>;

// TODO test whether it is more efficient to store all the pixels in a single 
// 1D pixel and use maths to determine row and collum 
// need to test this to find out whether it is faster or not 
// using a 2D vector simplifies the program and is more intuitive

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
    std::vector<std::vector<Pixel>>     m_pixelGrid;
    std::vector<std::vector<Vec2f>>     m_velocityField;
    std::vector<std::vector<FirePixel>> m_fireField;

    int m_gridWidth;
    int m_gridHeight;
    std::vector<SetPixelAction> m_SetPixelQueue;
    std::vector<sf::Uint8> m_buffer;

    std::vector<Action> m_actionQueue;


    FirePixel m_nullFire = material_properties::nullBurnProperties;



    std::random_device rd; 

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
        84, 72, 61, 255,

        // steel -> silver / blue grey
        132, 140, 140, 255,

        // wood brown
        173, 140, 99, 255,

        // oil
        145, 108, 4, 128
    };

    std::vector<sf::Uint8> m_fireColor = {189, 84, 40, 180};



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
    static T selectRandomElement(std::vector<T> & vec, std::mt19937 randomGen)  {
        int vectorLength = vec.size();
        if (vectorLength == 1) {
            return vec[0];
        }
        int index = std::uniform_int_distribution(0, vectorLength - 1) (randomGen);
        return vec[index];
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

    maybeResult<Vec2i> generalWaterPhysics(Vec2i currentPosition, std::mt19937 & randomGen) const {
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

        bool prefferedDirection = std::uniform_int_distribution<>(0,1)(randomGen);

        if (rightClear && leftClear) {
            if (right2Clear && left2clear) {
                return maybeResult(currentPosition + (prefferedDirection ? left : right));
            } else if (right2Clear) {
                return maybeResult(currentPosition + right2);
            } else if (left2clear) {
                return maybeResult(currentPosition + left2);
            }
            return maybeResult(currentPosition + (prefferedDirection ? left : right));
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

    maybeResult<Vec2i> generalFallingPhysics(Vec2i currentPosition) const {
        const Vec2i down = Vec2i(0,1);

        auto f_checkPos = [&currentPosition, this](Vec2i movement) {
            return (safeCheckIsAir(currentPosition + movement));
        };

        if (f_checkPos(down)) {
            return maybeResult(currentPosition + down);
        }
        else { return maybeResult<Vec2i>(); }
    }

    const Pixel getPixelConst(Vec2i pos) const {
        return m_pixelGrid[pos.x][pos.y];
    }

    Pixel& getPixelRef(Vec2i pos) {
        return m_pixelGrid[pos.x][pos.y];
    }

    maybeResult<m_IgniteAction> generalFirePhysics(Vec2i pos, std::mt19937 & randomGen) const {
        std::vector<Vec2i> neighborDelta = {Vec2i(1,0), Vec2i(1,-1), Vec2i(0,-1), Vec2i(-1,-1), Vec2i(-1,0), Vec2i(-1,1), Vec2i(0,1), Vec2i(1,1)};

        Pixel thisPixel = m_pixelGrid[pos.x][pos.y];
        if (bitop::flag_has_mask(thisPixel.properties, pixel_properties::OnFire)) {

            FirePixel burnProperties = material_properties::materialLookup[thisPixel.material].burnProperties;
            if (m_fireField[pos.x][pos.y].burnPercentageChance > std::uniform_int_distribution<>(0,99)(randomGen)) {
                Vec2i dPos = selectRandomElement(neighborDelta, randomGen);
                if (!isInBounds(pos + dPos)   ) {
                    return ( maybeResult<m_IgniteAction>() );
                }
                const Pixel candidatePixel = getPixelConst(pos + dPos);
                const uint8_t & candiateMaterial = candidatePixel.material;
                bool flammable = hasProperty(candiateMaterial, material_properties::Flammable);
                return (flammable && !bitop::flag_has_mask(candidatePixel.properties, pixel_properties::OnFire)
                ? maybeResult(m_IgniteAction{burnProperties, pos + dPos}) 
                : maybeResult<m_IgniteAction>());
            }
        }
        return maybeResult<m_IgniteAction>();
    }

    bool checkIsAir(Vec2i pos) const {
        return (m_pixelGrid[pos.x][pos.y].material == materials::air);
    };

    bool safeCheckIsAir(Vec2i pos) const {
        if (isInBounds(pos)) {
            return checkIsAir(pos);
        } else {return false;};
    }

    bool hasProperty(uint8_t material, material_properties::PixelFlags property) const {
        return bitop::flag_has_mask(material_properties::materialLookup[material].flags, property);
    }

    void doPhysics() {
        m_globalUpdateFrame ++;
        bool n2 = bitop::check_nth_bit(m_globalUpdateFrame, 1);
        std::mt19937 randomGen(rd());
        if (bitop::check_nth_bit(m_globalUpdateFrame, 0)) {
            for (int xi = 0; xi < m_gridWidth; xi++) {
                if (n2) {
                    for (int yi = 0; yi < m_gridHeight; yi ++) {
                        doPhysicsOnPixel(xi,yi, randomGen);
                    }
                } else {
                    for (int yi = m_gridHeight - 1; yi !=0; yi --) {
                        doPhysicsOnPixel(xi,yi, randomGen);
                    }
                }
            }
        } else {
            for (int xi = m_gridWidth - 1; xi !=0; xi --) {
                if (n2) {
                    for (int yi = 0; yi < m_gridHeight; yi ++) {
                        doPhysicsOnPixel(xi,yi, randomGen);
                    }
                } else {
                    for (int yi = m_gridHeight - 1; yi !=0; yi --) {
                        doPhysicsOnPixel(xi,yi, randomGen);
                    }
                }
            }
        }
    }

    void doPhysicsOnPixel(int col, int row, std::mt19937 & randomGen)
    {
        // the pixel must have a path to move through if it moves diagonally
        // othersie it seemingly phases through containers

        // assumption in all our next position functions, they cant returrn the same position they were given otherwise
        // that pixel would just be set to air, the maybe type is to represent the case that the function does not find
        // a new place to put the pixel

        Pixel currentPixel = m_pixelGrid[col][row];

        if (currentPixel.material == materials::air) { return; }

        if (currentPixel.updateFrame == m_globalUpdateFrame) { return; };

        Vec2i currentPos = Vec2i(col, row);
        maybeResult<Vec2i> nextPos;
        
        Vec2i v_nextPos;

        auto scopeCapturedSandPhysics = [&currentPos, this](){return this->generalSandPhysics(currentPos);};

        auto scopeCapturedWaterPhysics = [&randomGen, &currentPos, this](){return this->generalWaterPhysics(currentPos, randomGen);};


        maybeResult<m_IgniteAction> possibleIgnition;


        if (hasProperty(currentPixel.material, material_properties::fallingLiquid))
        {
            nextPos = maybeResult<Vec2i>()
                .tryWith(scopeCapturedSandPhysics)
                .tryWith(scopeCapturedWaterPhysics);
        }
        
        else if(hasProperty(currentPixel.material, material_properties::fallingPowder)) {
            nextPos = maybeResult<Vec2i>()
                .tryWith(scopeCapturedSandPhysics);
        }

        else if (hasProperty(currentPixel.material, material_properties::fixedSolid)) {
            nextPos = maybeResult<Vec2i>(); // essentially a nothing
        }

        else {
            throw std::runtime_error("UnknownMaterial fix in doPhysics method of pixelgrid class in PixelGridd.hpp");
        } 

        if (nextPos.exists()) {
            v_nextPos = nextPos.getValue();
            swapPixels(currentPos, v_nextPos);
        } else {
            v_nextPos = currentPos;
        }
        currentPos = v_nextPos;

        auto scopeCaptureFirePhysics = [&randomGen, &currentPos, this](){return this->generalFirePhysics(currentPos, randomGen);};

        if (bitop::flag_has_mask(currentPixel.properties, pixel_properties::OnFire)) {
            possibleIgnition = maybeResult<m_IgniteAction>()
                .tryWith(scopeCaptureFirePhysics);

            m_fireField[currentPos.x][currentPos.y].burnTime -= 1;
            if ((m_fireField[currentPos.x][currentPos.y].burnTime) == 0) {
                setPixel({currentPos, {0,0,0}});
                m_fireField[currentPos.x][currentPos.y] = material_properties::nullBurnProperties;
            } else if (possibleIgnition.exists()) {
                m_Ignition(possibleIgnition.getValue());
            }
            
        }


        m_pixelGrid[currentPos.x][currentPos.y].updateFrame = m_globalUpdateFrame;
    }

    void m_Ignition(m_IgniteAction action) {
        getPixelRef(action.pos).properties |= pixel_properties::OnFire;     

        m_fireField[action.pos.x][action.pos.y] = action.fireProperties;
    }

    void swapPixels(Vec2i pos1, Vec2i pos2) {
        Pixel pixel2 = getPixelConst(pos2);
        m_pixelGrid[pos2.x][pos2.y] = m_pixelGrid[pos1.x][pos1.y];
        m_pixelGrid[pos1.x][pos1.y] = pixel2;
        FirePixel fire2 = m_fireField[pos1.x][pos1.y];

        m_fireField[pos2.x][pos2.y] = m_fireField[pos1.x][pos1.y];
        m_fireField[pos1.x][pos1.y] = fire2;
    }

    void init()
    {
        initRandom();
        initGrid();
    }

    void initRandom()
    {
    }

    void initGrid()
    {
        // TODO overload the function to take a config file name
        // and load all the pixels from that config file
        // currently this sets all the pixels to air 
        m_pixelGrid.assign(m_gridWidth, std::vector<Pixel>(m_gridHeight, Pixel{materials::air, 0, 0}));
        m_fireField.assign(m_gridWidth, std::vector<FirePixel>(m_gridHeight, m_nullFire));
    }
    
    void tryIgnitePixel(IgnitionAction action) {

        Pixel & pixel = getPixelRef(action.pos);
        if (hasProperty(pixel.material, material_properties::Flammable) && !bitop::flag_has_mask(pixel.properties, pixel_properties::OnFire)) 
        {
            pixel.properties |= pixel_properties::OnFire;   
            FirePixel burnProperties = material_properties::materialLookup[pixel.material].burnProperties;
            m_IgniteAction immolate = {burnProperties, action.pos}; 
            m_Ignition(immolate);
        };
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
            } else if (std::holds_alternative<DrawParallelogramAction>(*a)) {
                drawParallelogram(std::get<DrawParallelogramAction>(*a));
            } else if (std::holds_alternative<IgnitionAction>(*a)) {
                tryIgnitePixel(std::get<IgnitionAction>(*a));
            }
            a = actions.erase(a);
        }
    }

    void drawParallelogram(DrawParallelogramAction action) {
        std::vector<Vec2i> topLine = getLine(action.cornerTL, action.cornerTR);
        Vec2i TLtoBL = action.cornerBL - action.cornerTR;
        for (Vec2i pixelPos : topLine) {
            DrawLineAction line = {pixelPos, pixelPos + TLtoBL, 1,action.pixel_type};
            drawLine(line);
        }
    }

    void setPixel(SetPixelAction action) 
    {
        action.pixel_type.properties = pixel_properties::None;
        m_pixelGrid[action.pos.x][action.pos.y] = action.pixel_type;       

        m_fireField[action.pos.x][action.pos.y] = m_nullFire;
    }


    void trySetPixel(SetPixelAction action)
    {
        if (isInBounds(action.pos)) {
            setPixel(action);
        };
    }

    std::vector<Vec2i> getLine(Vec2i start, Vec2i end) {
        std::vector<Vec2i> resultingPath = {};

        // Extract start and end points
        int x0 = start.x;
        int y0 = start.y;
        int x1 = end.x;
        int y1 = end.y;

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
            resultingPath.push_back(Vec2i{x, y});
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
        return resultingPath;
    }

    void drawLine(DrawLineAction action)
    {
        std::vector<Vec2i> path = getLine(action.start, action.end);
        for (Vec2i pos : path) {
            drawCircle(DrawCircle{.pos=pos, .radius=action.width, .pixel_type=action.pixel_type});
        }
    }

    void drawCircle(DrawCircle action)
    {
        Vec2i pos = action.pos;
        int radius = action.radius;
        
        for (int x = pos.x - radius; x  <= pos.x + radius; x ++) 
        {
            for (int y = pos.y - radius; y <= pos.y + radius; y ++) 
            {
                Vec2i pos2 = Vec2i(x,y);
                if (pos.distsq(pos2) <= pow(radius, 2)) {
                    trySetPixel(SetPixelAction{pos2, action.pixel_type});
                }           
            }
        }
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
                if (! bitop::flag_has_mask(m_pixelGrid[i][j].properties, pixel_properties::OnFire)) {
                    m_buffer[p  ] = m_materialToColor_c[4 * m_pixelGrid[i][j].material];
                    m_buffer[p+1] = m_materialToColor_c[4 * m_pixelGrid[i][j].material + 1];
                    m_buffer[p+2] = m_materialToColor_c[4 * m_pixelGrid[i][j].material + 2];
                    m_buffer[p+3] = m_materialToColor_c[4 * m_pixelGrid[i][j].material + 3];
                } else {
                    m_buffer[p  ] = m_fireColor[0];
                    m_buffer[p+1] = m_fireColor[1]; 
                    m_buffer[p+2] = m_fireColor[2];
                    m_buffer[p+3] = m_fireColor[3];
                }
            }
        }
    }


public:
    PixelGrid() : m_gridWidth(0), m_gridHeight(0) {}

    PixelGrid(int width, int height) 
        : m_gridWidth(width), m_gridHeight(height),
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

        doPhysics();
        executeActions(m_actionQueue);
        updateDrawBuffer();

        /*std::cout << "done draw buffer\n";*/
    }

    void userAction(Action userAction) {
        m_actionQueue.push_back(userAction);
    }

    auto& getPixels() {
        return m_buffer;
    }
};
