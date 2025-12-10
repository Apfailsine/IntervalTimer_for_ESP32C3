#include "board.h"

BoardService::BoardService()
{
}

/*!
 *  @brief - sets hardware consts like pins/pinouts, resistors, etc. depending on the electronics version and sensor type
 *  @brief - also turns on the measuring current
 */


const std::array<ButtonState, 2> *BoardService::getButtons()
{
    unsigned long curMil = millis(); // Get the current time

    //* Handle SW1
    if (digitalRead(SW1_PIN) == LOW) // Button is pressed
    {
        if (!sw1Pressed_) // on first iteration after button press was registered
        {
            sw1Pressed_ = true;
            pressStartSW1_ = curMil; // Record press start time
            if (!sw1ShortPressHandled_)
            {
                buttonState_[0] = ButtonState::SHORT_PRESS;
                sw1ShortPressHandled_ = true;
            }
        }
        // If button was pressed longer than LONG_PRESS_THRESHOLD_MS, go into this if-case (once)
        else if (((curMil - pressStartSW1_) >= LONG_PRESS_THRESHOLD_MS) && !sw1LongPressHandled_)
        {
            buttonState_[0] = ButtonState::LONG_PRESS;
            sw1LongPressHandled_ = true;
        }
        // If button was pressed longer than EXTENDED_PRESS_THRESHOLD_MS, go into this if-case (once)
        else if (((curMil - pressStartSW1_) >= EXTENDED_PRESS_THRESHOLD_MS) && !sw1ExtendedPressHandled_)
        {
            buttonState_[0] = ButtonState::EXTRA_LONG_PRESS;
            sw1ExtendedPressHandled_ = true;
        }
        else
        {
            buttonState_[0] = ButtonState::NO_PRESS; // If no change in ButtonState, return No_PRESS
        }
    }
    //* reset logic-bools on release
    else // Button not pressed
    {
        if (sw1Pressed_) // On immediate release (it was pressed in the previous getButtons() call)
        {
            sw1Pressed_ = false;
            sw1ShortPressHandled_ = false;
            sw1LongPressHandled_ = false;
            sw1ExtendedPressHandled_ = false;
            buttonState_[0] = ButtonState::NO_PRESS; // If no change in ButtonState, return NO_PRESS
        }
    }

    //* Handle SW2
    if (digitalRead(SW2_PIN) == LOW) // Button is pressed
    {
        if (!sw2Pressed_) // on first iteration after button press was registered
        {
            sw2Pressed_ = true;
            pressStartSW2_ = curMil; // Record press start time
            if (!sw2ShortPressHandled_)
            {
                buttonState_[1] = ButtonState::SHORT_PRESS;
                sw2ShortPressHandled_ = true;
            }
        }
        // If button was pressed longer than LONG_PRESS_THRESHOLD_MS, go into this if-case (once)
        else if (((curMil - pressStartSW2_) >= LONG_PRESS_THRESHOLD_MS) && !sw2LongPressHandled_)
        {
            buttonState_[1] = ButtonState::LONG_PRESS;
            sw2LongPressHandled_ = true;
        }
        // If button was pressed longer than EXTENDED_PRESS_THRESHOLD_MS, go into this if-case (once)
        else if (((curMil - pressStartSW2_) >= EXTENDED_PRESS_THRESHOLD_MS) && !sw2ExtendedPressHandled_)
        {
            buttonState_[1] = ButtonState::EXTRA_LONG_PRESS;
            sw2ExtendedPressHandled_ = true;
        }
        else
        {
            buttonState_[1] = ButtonState::NO_PRESS; // If no change in ButtonState, return NO_PRESS
        }
    }
    //* reset logic-bools on release
    else // Button not pressed
    {
        if (sw2Pressed_) // On immediate release (it was pressed in the previous getButtons() call)
        {
            sw2Pressed_ = false;
            sw2ShortPressHandled_ = false;
            sw2LongPressHandled_ = false;
            sw2ExtendedPressHandled_ = false;
            buttonState_[1] = ButtonState::NO_PRESS; // If no change in ButtonState, return NO_PRESS
        }
    }

    return &buttonState_;
}