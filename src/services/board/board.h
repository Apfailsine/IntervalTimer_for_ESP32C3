#ifndef BOARD_H
#define BOARD_H

#include <Arduino.h>
#include <string>
#include <array>
// models
#include "models/datastructures.h"

class BoardService
{
public:
    BoardService();

    const ButtonState *getButtons(); // TODO: consider returntype to be const reference


private:

    ButtonState buttonState_;
    
    // pin assignment, defined in board.cpp
    static constexpr int kButtonPin = 10; // D10 on XIAO ESP32C3 = GPIO10
    int SW1_PIN = -1;        // Pin connected to Switch1
    int SW2_PIN = -1;        // Pin connected to Switch1
    
    // Buttons
    static constexpr int LONG_PRESS_THRESHOLD_MS = 1000;
    static constexpr int EXTENDED_PRESS_THRESHOLD_MS = 3000;
    bool sw1Pressed_ = false;
    bool sw1EventPending_ = false;
    unsigned long pressStartSW1_ = 0;
    int pressStartSW2_ = 0;

};

#endif // BOARD_H
