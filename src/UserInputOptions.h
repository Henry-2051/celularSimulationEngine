#pragma once

namespace inputModes {

constexpr int paintBrush = 0;
constexpr int holdAndDragLine = 1;
constexpr int drawCircle = 2;
constexpr int drawParallelogram= 3;
constexpr int ignitePixel = 4;

const int numModes = 5;

constexpr int inputModes[numModes] = { 
    paintBrush, 
    holdAndDragLine, 
    drawCircle,
    drawParallelogram,
    ignitePixel,
};

constexpr const char* inputModeNames[numModes] = {
    "paint brush",
    "draw line",
    "draw circle",
    "draw parallelogram",
    "ignite pixel"
};
}
