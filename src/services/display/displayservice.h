#pragma once
#include "models/datastructures.h"

#include <Arduino.h>
#include <U8g2lib.h>
#include <array>

class DisplayService {
public:
    bool begin();
    void showBootScreen();
    void showStatus(const char* line1, const char* line2 = nullptr, const char* line3 = nullptr);
    void showCountdown(const char* label, unsigned long timeMillis);
    void clear();

    void chooseExercise(const Exercise* exercise, WifiState wifiState);
    void playTimer (const Exercise* exercise, unsigned long timeMillis, const ExerciseRuntime& runtime, WifiState wifiState);
    void configureLine(uint8_t line, uint8_t baseline, const uint8_t* font = nullptr);
    void setLine(uint8_t line, const String& text, bool autoRefresh = true);
    void setLine(uint8_t line, const String& text, const uint8_t* font, uint8_t baseline, bool autoRefresh = true);
    void printText(uint8_t lineFrom, uint8_t lineTo, const String& text);
    void refresh();

private:
    static constexpr uint8_t kSdaPin = 6; // SDA on D4
    static constexpr uint8_t kSclPin = 7; // SCL on D5
    static constexpr uint8_t kDefaultLineCount = 5;
    static constexpr const uint8_t* kDefaultFont = u8g2_font_ncenB08_tr;

    void clearLines();
    void resetLayout();
    void setLineInternal(uint8_t line, const String& text);
    void setLineInternal(uint8_t line, const String& text, const uint8_t* font, uint8_t baseline);
    void render();

    U8G2_SSD1306_128X64_NONAME_F_HW_I2C display_{U8G2_R0, U8X8_PIN_NONE};
    struct Line {
        String text;
        const uint8_t* font = kDefaultFont;
        uint8_t baseline = 16;
        bool visible = false;
    };
    std::array<Line, kDefaultLineCount> lines_{};
    bool dirty_ = false;
};
