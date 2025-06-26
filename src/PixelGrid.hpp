#include "Materials.h"
#include "Vec2.hpp"
// #include "imgui-SFML.h"
#include <SFML/Config.hpp>
#include <cmath>
#include <cstddef>
#include <cstdint>
// #include <iostream>
// #include <memory>
#include <iostream>
#include <random>
#include <stdexcept>
#include <strings.h>
#include <sys/types.h>
#include <utility>
#include <variant>
#include <vector>
#include "BitOperationHelpers.hpp"
#include "maybeResult.hpp"
#include "PixelGridContainer.hpp"


struct SetPixelAction;

struct DrawCircle;

struct DrawLineAction;

struct DrawParallelogramAction;

struct m_IgniteAction;

struct IgnitionAction;

struct ActionPair;

struct IncinerationAction;

struct m_IgniteAction;

struct SetPixelMaterialAndProperties;

using Action = std::variant<SetPixelAction, DrawCircle, DrawLineAction, DrawParallelogramAction, IgnitionAction, IncinerationAction, m_IgniteAction, SetPixelMaterialAndProperties>;

using ActionIncludingPair = std::variant<Action, ActionPair>;
struct Pixel 
{
    uint8_t material;
    uint8_t updateFrame;
    uint8_t properties;
};

using FirePixel = material_properties::ignitionProperties;

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


struct m_IgniteAction
{
    FirePixel fireProperties;
    Vec2i     pos;
};

struct IgnitionAction {
    Vec2i pos;
};

struct IncinerationAction {
    Vec2i pos;
    Pixel pixel;
};


struct SetPixelMaterialAndProperties{
    Vec2i pos;
    uint8_t material;
    uint8_t properties;
};

struct ActionPair {
    std::pair<Action, Action> this_action;
};


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
const sf::Color MetalGray(127, 128, 128);           // Metal surfaces



class PixelGrid
{
    PixelGridContainer<Pixel>     m_pixelGrid;
    PixelGridContainer<Vec2f>     m_velocityField;
    PixelGridContainer<FirePixel> m_fireField;

    int m_gridWidth;
    int m_gridHeight;
    std::vector<SetPixelAction> m_SetPixelQueue;
    std::vector<sf::Uint8> m_buffer;

    std::vector<ActionIncludingPair> m_actionQueue;


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
        145, 108, 4, 128,

        // coal
        94, 94, 94, 255,

        // ash
        166, 166, 166, 255,

        // steam
        145, 176, 194, 255,

        // lava 
        89, 28, 10, 255,

        // obsidian 
        28, 18, 15, 255,
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
        size_t vectorLength = vec.size();
        if (vectorLength == 1) {
            return vec[0];
        }
        size_t index = std::uniform_int_distribution<size_t>(0, vectorLength - 1) (randomGen);
        return vec[index];
    }

    maybeResult<Vec2i> generalSandPhysics(Vec2i currentPosition) const {
        const Vec2i down = Vec2i(0,1);
        const Vec2i downRight = Vec2i(1,1);
        const Vec2i downLeft = Vec2i(-1,1);
        const Vec2i right = Vec2i(1,0);
        const Vec2i left = Vec2i(-1,0);

        auto f_checkPos = [&currentPosition, this](Vec2i movement) {
            return (safeShouldSwapGasses(currentPosition, movement));
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

    bool isDenserThanTarget(int density, Vec2i target) const {
        int targetDensity = material_properties::materialLookup[getPixelConst(target).material].density;

        return (density > targetDensity);
    }

    bool safeShouldSwapGasses(Vec2i currentPosition, Vec2i movement) const {
        if(!isInBounds(currentPosition + movement)){
            return false;
        };

        Pixel currentPixel = getPixelConst(currentPosition);
        Pixel targetPixel = getPixelConst(currentPosition + movement);
        if (hasProperty(targetPixel.material, material_properties::IsLiquid)) {
            return (isDenserThanTarget(material_properties::materialLookup[currentPixel.material].density, currentPosition + movement));
        } else {
            return (checkIsGas(currentPosition + movement));
        }
    }

    maybeResult<Vec2i> generalGasMovementHorizontalDelta(Vec2i currentPosition, std::mt19937 & randomGen, Pixel pixelToMove) const {
        const Vec2i right = {1,0};
        const Vec2i left = {-1,0};
        const Vec2i right2 = 2 * right;
        const Vec2i left2 = 2 * left; 

        auto f_checkPos = [&pixelToMove, &currentPosition, this](Vec2i movement) {
            if (!isInBounds(currentPosition + movement)) { return false; }

            const Pixel& candidatePixel = getPixelConst(currentPosition + movement);

            if (pixelToMove.material != candidatePixel.material && hasProperty(candidatePixel.material, material_properties::IsGas)) {
                return true;
            }
            return false;
        };

        if (std::uniform_int_distribution<>(0, 1)(randomGen)) {
            if (f_checkPos(right)) 
            {
                return f_checkPos(right2) ? maybeResult(right2) : maybeResult(right);
            } 
            else if (f_checkPos(left))
            {
                return f_checkPos(left2) ? maybeResult(left2) : maybeResult(left);
            }
        } else 
        {
            if (f_checkPos(left))
            {
                return f_checkPos(left2) ? maybeResult(left2) : maybeResult(left);
            }
            else if (f_checkPos(right)) 
            {
                return f_checkPos(right2) ? maybeResult(right2) : maybeResult(right);
            } 
        }
        return maybeResult<Vec2i>();
    }

    maybeResult<Vec2i> generalGasMovementVerticalDelta(Vec2i currentPosition, Pixel pixelToMove) const {
        const Vec2i up = {0,-1};


        auto f_checkPosGas = [&pixelToMove, &currentPosition, this](Vec2i movement) {
            if (!isInBounds(currentPosition + movement)) {
                return false;
            }
            const Pixel& candidatePixel = getPixelConst(currentPosition + movement);
            return (pixelToMove.material != candidatePixel.material && hasProperty(candidatePixel.material, material_properties::IsGas));
        };

        auto f_checkPosDensityUnsafe = [&pixelToMove, &currentPosition, this](Vec2i movement) {
            const Pixel& candidatePixel = getPixelConst(currentPosition + movement);

            return (material_properties::materialLookup[pixelToMove.material].density < material_properties::materialLookup[candidatePixel.material].density);
        };

        auto f_checkPos = [&f_checkPosGas, &f_checkPosDensityUnsafe](Vec2i movement) {return f_checkPosGas(movement) && f_checkPosDensityUnsafe(movement);};

        return (f_checkPos(up) ? maybeResult(up) : maybeResult<Vec2i>());
    }

    maybeResult<Vec2i> generalWaterPhysics(Vec2i currentPosition, std::mt19937 & randomGen) const {
        const Vec2i right = Vec2i(1,0);
        const Vec2i right2 = Vec2i(2,0);
        const Vec2i left = Vec2i(-1,0);
        const Vec2i left2 = Vec2i(-2,0);
        const Vec2i down = Vec2i(0,1);

        auto f_checkPos = [&currentPosition, this](Vec2i movement) {
            return (safeCheckIsGas(currentPosition + movement));
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
            return (safeCheckIsGas(currentPosition + movement));
        };

        if (f_checkPos(down)) {
            return maybeResult(currentPosition + down);
        }
        else { return maybeResult<Vec2i>(); }
    }

    const Pixel& getPixelConst(Vec2i pos) const {
        return m_pixelGrid[pos.x][pos.y];
    }



    Pixel& getPixelRef(Vec2i pos) {
        return m_pixelGrid[pos.x][pos.y];
    }

    maybeResult<ActionIncludingPair> generalFireSpreadPhysics(Vec2i pos, std::mt19937 & randomGen) const {
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

    constexpr std::array<Vec2i, 48> neighboringDeltas() const {
        int deltas[7] = {-3,-2,-1,0,1,2,3};

        size_t count = 0;
        std::array<Vec2i, 48> neighbors;
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                if ((i==0) && (j==0)) {continue;}
                neighbors[count] = Vec2i(deltas[i], deltas[j]);
                count ++;
            }
        }

        return neighbors;
     }
    
    maybeResult<ActionIncludingPair> generalFireInteractionPhysics(Vec2i pos) {
        std::array<Vec2i, 48> neighbors = neighboringDeltas();

        Pixel thisPixel = m_pixelGrid[pos.x][pos.y];
        for (Vec2i neighborDelta : neighbors) {
            Vec2i targetPos= neighborDelta + pos;
            if(!isInBounds(targetPos)) {continue;};
            Pixel targetPixel = m_pixelGrid[targetPos.x][targetPos.y];
            auto reaction = material_reactions::on_fire_reactions.find(std::pair(thisPixel.material, targetPixel.material));
            if (reaction != material_reactions::on_fire_reactions.end()) {
                SetPixelMaterialAndProperties a1 = {pos, reaction->second.first.material, reaction->second.first.properties};
                SetPixelMaterialAndProperties a2 = {targetPos, reaction->second.second.material, reaction->second.second.properties};
                return maybeResult(ActionPair({a1, a2}));
            }
        };
        return maybeResult<ActionIncludingPair>();
    }


    bool checkIsGas(Vec2i pos) const {
        return (hasProperty(m_pixelGrid[pos.x][pos.y].material, material_properties::IsGas));
    };

    bool safeCheckIsGas(Vec2i pos) const {
        if (isInBounds(pos)) {
            return checkIsGas(pos);
        } 
        return false;
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
                        doPhysicsOnPixel(xi, yi, randomGen);
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
        maybeResult<Vec2i> nextPos = maybeResult<Vec2i>();
        maybeResult<Vec2i> nextNextPos = maybeResult<Vec2i>();
        
        auto scopeCapturedSandPhysics = [&currentPos, this](){return this->generalSandPhysics(currentPos);};

        auto scopeCapturedWaterPhysics = [&randomGen, &currentPos, this](){return this->generalWaterPhysics(currentPos, randomGen);};




        // pixel movement doesnt change the type of pixels
        if (hasProperty(currentPixel.material, material_properties::fallingLiquid))
        {
            nextPos = scopeCapturedSandPhysics().tryWith(scopeCapturedWaterPhysics);

            currentPos = swapIfExists(currentPos, nextPos);
        }
        
        else if(hasProperty(currentPixel.material, material_properties::fallingPowder)) {
            nextPos = scopeCapturedSandPhysics();

            currentPos = swapIfExists(currentPos, nextPos);
        }

        else if (hasProperty(currentPixel.material, material_properties::IsSolid)) {
            nextPos = maybeResult<Vec2i>(); // essentially a nothing
            
            currentPos = swapIfExists(currentPos, nextPos);
        } 

        else if (hasProperty(currentPixel.material, material_properties::IsGas)) {
            nextPos = maybeResult<Vec2i>(currentPos) + generalGasMovementHorizontalDelta(currentPos, randomGen, currentPixel);

            nextNextPos = nextPos + generalGasMovementVerticalDelta(nextPos.getValue(), currentPixel);

            currentPos = swapIfExists(currentPos, nextNextPos);

            // currentPos = swapIfExists(nextPos.getValue(), nextNextPos);
        }
        else {
            throw std::runtime_error("UnknownMaterial fix in doPhysics method of pixelgrid class in PixelGridd.hpp");
        } 

        auto scopeCapturedFireSpreadPhysics= [&randomGen, &currentPos, this](){return this->generalFireSpreadPhysics(currentPos, randomGen);};
        auto scopeCapturedIncinerationPhysics = [&currentPos, this](){return this->incinerationCheck(currentPos);};
        auto scopeCapturedFireInteractionPhysics=  [&currentPos, this](){return this->generalFireInteractionPhysics(currentPos);}; 

        if (bitop::flag_has_mask(currentPixel.properties, pixel_properties::OnFire)) {
            if (!bitop::flag_has_mask(currentPixel.properties, pixel_properties::AlwaysOnFire)) {
                m_fireField[currentPos.x][currentPos.y].burnTime -= 1;
            }
            // maybeResult<Action> fireResult = scopeCapturedIncinerationPhysics().tryWith(scopeCapturedFireSpreadPhysics);
            // maybeResult<Action> fireResult = scopeCapturedFireSpreadPhysics();

            maybeResult<ActionIncludingPair> fireResult = scopeCapturedIncinerationPhysics().tryWith(scopeCapturedFireInteractionPhysics).tryWith(scopeCapturedFireSpreadPhysics);
            if (fireResult.exists()) {
                executeAction(fireResult.getValue());
            }
            currentPixel = m_pixelGrid[currentPos.x][currentPos.y];

        }

        m_pixelGrid[currentPos.x][currentPos.y].updateFrame = m_globalUpdateFrame;
    }

    Vec2i swapIfExists(Vec2i currentPos, maybeResult<Vec2i> nextPos) {
        Vec2i v_nextPos;
        if (nextPos.exists()) {
            v_nextPos = nextPos.getValue();
            swapPixels(currentPos, v_nextPos);
        } else {
            v_nextPos = currentPos;
        }
        return v_nextPos;
    }

    void m_incineratePixel(IncinerationAction action) {
        Pixel currentPixel = m_pixelGrid[action.pos.x][action.pos.y];
        uint8_t newMaterial = materials::incineration_table[currentPixel.material];
        currentPixel.material = newMaterial;

        setPixel({action.pos, currentPixel}); 
    }

    maybeResult<ActionIncludingPair> incinerationCheck(Vec2i pos) const {
        if (m_fireField[pos.x][pos.y].burnTime == 0) {
            return maybeResult<IncinerationAction>({pos, m_pixelGrid[pos.x][pos.y]});
        }
        return maybeResult<IncinerationAction>();
    }

    void m_Ignition(m_IgniteAction action) {
        getPixelRef(action.pos).properties |= pixel_properties::OnFire;     

        m_fireField[action.pos.x][action.pos.y] = action.fireProperties;
    }

    void swapPixels(Vec2i pos1, Vec2i pos2) {
        Pixel pixel2 = getPixelConst(pos2);
        if(!isInBounds(pos1) || !isInBounds(pos2)) {
            throw std::runtime_error("trying to swap not in bounds");
        }
        m_pixelGrid[pos2.x][pos2.y] = m_pixelGrid[pos1.x][pos1.y];
        m_pixelGrid[pos1.x][pos1.y] = pixel2;
        // m_pixelGrid[pos2.x][pos2.y].updateFrame = m_globalUpdateFrame;

        FirePixel fire2 = m_fireField[pos2.x][pos2.y];
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
        m_pixelGrid = PixelGridContainer<Pixel>(m_gridWidth, m_gridHeight, Pixel{materials::air, 0, 0});
        m_fireField = PixelGridContainer<FirePixel>(m_gridWidth, m_gridHeight, m_nullFire);
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

    void doActionPair(ActionPair actions) {
        _executeAction(actions.this_action.first);

        _executeAction(actions.this_action.second);
    }


    void executeAction(ActionIncludingPair a) {
        if (std::holds_alternative<ActionPair>(a)) {
            doActionPair(std::get<ActionPair>(a));
        } else {
            _executeAction(std::get<Action>(a));
        }
    }

    void _executeAction(Action a) {
        if (std::holds_alternative<SetPixelAction>(a)) {
            setPixel(std::get<SetPixelAction>(a));
        } else if (std::holds_alternative<DrawLineAction>(a)) {
            drawLine(std::get<DrawLineAction>(a));
        } else if (std::holds_alternative<DrawCircle>(a)) {
            drawCircle(std::get<DrawCircle>(a));
        } else if (std::holds_alternative<DrawParallelogramAction>(a)) {
            drawParallelogram(std::get<DrawParallelogramAction>(a));
        } else if (std::holds_alternative<IgnitionAction>(a)) {
            tryIgnitePixel(std::get<IgnitionAction>(a));
        } else if (std::holds_alternative<IncinerationAction>(a)) {
            m_incineratePixel(std::get<IncinerationAction>(a));
        } else if (std::holds_alternative<m_IgniteAction>(a)) {
            m_Ignition(std::get<m_IgniteAction>(a));
        } else if (std::holds_alternative<SetPixelMaterialAndProperties>(a) ) { 
            m_SetPixelMaterialAndProperties(std::get<SetPixelMaterialAndProperties>(a));
        }
        else {
            throw std::runtime_error("Tried to execute action but execution function is not defined (executeAction)\n");
        }
    }

    void executeActions(std::vector<ActionIncludingPair> & actions) 
    {
        /*std::cout << "Size of queue : " << actions.size() << "\n";*/
        for (auto a = actions.begin(); a != actions.end();) {
            executeAction(*a);
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
        action.pixel_type.properties = pixel_properties::DefaultMaterialProperties[action.pixel_type.material];

        m_pixelGrid[action.pos.x][action.pos.y] = action.pixel_type;       

        material_properties::ignitionProperties newBurnProperties = material_properties::materialLookup[action.pixel_type.material].burnProperties;

        m_fireField[action.pos.x][action.pos.y] = newBurnProperties;
    }

    void m_SetPixelMaterialAndProperties(SetPixelMaterialAndProperties action) {
        m_pixelGrid[action.pos.x][action.pos.y].material = action.material;
        m_pixelGrid[action.pos.x][action.pos.y].properties = action.properties;
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
        m_buffer.assign(static_cast<size_t>(m_gridWidth * m_gridHeight * 4), static_cast<size_t>(0));
        for (size_t i = 0; i < static_cast<size_t>(m_gridWidth); i++) 
        {
            for (size_t j = 0; j < static_cast<size_t>(m_gridHeight); j ++)
            {
                size_t q =  (j * static_cast<size_t>(m_gridWidth) + i);
                size_t p = q * 4;
                if ((! bitop::flag_has_mask(m_pixelGrid[i][j].properties, pixel_properties::OnFire)) || (m_pixelGrid[i][j].material == materials::lava)) {
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
        m_buffer(static_cast<size_t>(width * height * 4))
    {
        init();
    }

    // void reset(int width, int height) 
    // {
    //     m_gridWidth = width;
    //     m_gridHeight = height;
    //     m_buffer.resize(width * height * 4);
    //
    //     init();
    // }


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

    void userAction(ActionIncludingPair userAction) {
        m_actionQueue.push_back(userAction);
    }

    auto& getPixels() {
        return m_buffer;
    }
};
