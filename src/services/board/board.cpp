#include "board.h"

BoardService::BoardService()
    : buttonState_(ButtonState::NO_PRESS)
{
    SW1_PIN = kButtonPin;
    pinMode(kButtonPin, INPUT_PULLUP); // enable internal pull-up for button on D10
}

/*!
 *  @brief - sets hardware consts like pins/pinouts, resistors, etc. depending on the electronics version and sensor type
 *  @brief - also turns on the measuring current
 */


const ButtonState *BoardService::getButtons()
{
    unsigned long curMil = millis();

    if (sw1EventPending_)
    {
        sw1EventPending_ = false;
        buttonState_ = ButtonState::NO_PRESS;
    }

    if (digitalRead(SW1_PIN) == LOW)
    {
        if (!sw1Pressed_)
        {
            sw1Pressed_ = true;
            pressStartSW1_ = curMil;
        }
        return &buttonState_;
    }

    if (sw1Pressed_)
    {
        sw1Pressed_ = false;
        unsigned long pressDuration = curMil - pressStartSW1_;
        if (pressDuration >= EXTENDED_PRESS_THRESHOLD_MS)
        {
            buttonState_ = ButtonState::EXTRA_LONG_PRESS;
            Serial.println("BoardService - getButtons EXTRA_LONG_PRESS detected");
        }
        else if (pressDuration >= LONG_PRESS_THRESHOLD_MS)
        {
            buttonState_ = ButtonState::LONG_PRESS;
            Serial.println("BoardService - getButtons LONG_PRESS detected");
        }
        else
        {
            buttonState_ = ButtonState::SHORT_PRESS;
            Serial.println("BoardService - getButtons SHORT_PRESS detected");
        }
        sw1EventPending_ = true;
    }

    return &buttonState_;
}