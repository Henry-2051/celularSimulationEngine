#pragma once

namespace inputModes {

constexpr int paintBrush = 0;
constexpr int holdAndDragLine = 1;
constexpr int drawCircle = 2;
constexpr int drawParallelogram= 3;

const int numModes = 4;

constexpr int inputModes[numModes] = { 
    paintBrush, 
    holdAndDragLine, 
    drawCircle,
    drawParallelogram
};

constexpr const char* inputModeNames[numModes] = {
    "paint brush",
    "draw line",
    "draw circle",
    "draw parallelogram"
};
}
