#pragma once

namespace inputModes {

constexpr int paintBrush = 0;
constexpr int holdAndDragLine = 1;
constexpr int drawCircle = 2;

const int numModes = 3;

constexpr int inputModes[numModes] = { 
    paintBrush, 
    holdAndDragLine, 
    drawCircle};

constexpr const char* inputModeNames[numModes] = {
    "paint brush",
    "draw line",
    "draw circle"
};
}
