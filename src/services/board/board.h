#ifndef BOARD_H
#define BOARD_H

#include <Arduino.h>
#include <string>
#include <array>
// models
#include "datastructures.h"

class BoardService
{
public:
    BoardService();

    const std::array<ButtonState, 2> *getButtons(); // TODO: consider returntype to be const reference


private:


    
    std::array<ButtonState, 2> buttonState_;
    

    // pin assignment, defined in board.cpp
    int SW1_PIN = -1;        // Pin connected to Switch1
    int SW2_PIN = -1;        // Pin connected to Switch1
    

    
    // Buttons
    static constexpr int LONG_PRESS_THRESHOLD_MS = 1000;
    static constexpr int EXTENDED_PRESS_THRESHOLD_MS = 3000;
    bool sw1Pressed_ = false;
    bool sw2Pressed_ = false;
    bool sw1ShortPressHandled_ = false;
    bool sw2ShortPressHandled_ = false;
    bool sw1LongPressHandled_ = false;
    bool sw2LongPressHandled_ = false;
    bool sw1ExtendedPressHandled_ = false;
    bool sw2ExtendedPressHandled_ = false;
    int pressStartSW1_ = 0;
    int pressStartSW2_ = 0;

};

#endif // BOARD_H
