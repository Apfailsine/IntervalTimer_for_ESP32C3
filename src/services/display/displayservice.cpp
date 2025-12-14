#include "displayservice.h"
#include "models/datastructures.h"

#include <Wire.h>
#include <algorithm>
#include <array>
#include <cstdio>

bool DisplayService::begin() {
    Wire.begin(kSdaPin, kSclPin);
    display_.begin();
    display_.setFont(kDefaultFont);
    display_.setFontMode(1);
    display_.setDrawColor(1);
    resetLayout();
    clearLines();
    render();
    return true;
}

void DisplayService::showBootScreen() {
    clearLines();
    setLineInternal(0, String("IntervalTimer"));
    setLineInternal(1, String("Display bereit"));
    render();
}

void DisplayService::showStatus(const char* line1, const char* line2, const char* line3) {
    clearLines();
    if (line1) {
        setLineInternal(0, String(line1));
    }
    if (line2) {
        setLineInternal(1, String(line2));
    }
    if (line3) {
        setLineInternal(2, String(line3));
    }
    render();
}

void DisplayService::clear() {
    clearLines();
    render();
}

void DisplayService::chooseExercise(const Exercise* exercise, WifiState wifiState) {
    String exerciseName = exercise ? String(exercise->name.c_str()) : String("Keine Uebung gewählt");
    const char* wifiText = (wifiState == WifiState::ACTIVE) ? "WiFi: Aktiv" : "WiFi: Inaktiv";
    
    configureLine(0, 20, u8g2_font_crox1tb_tf );   // 9px Schrift
    configureLine(1, 46, u8g2_font_crox5tb_tf);     // 16px Schrift
    configureLine(2, 64, u8g2_font_crox1tb_tf );     // 9px für WiFi-Status
    setLine(0, "Uebung", false);
    setLine(1, exerciseName, false);
    setLine(2, String("                   ") + wifiText, false);
    refresh();
}

void DisplayService::playTimer(const Exercise* exercise, unsigned long timeMillis, const ExerciseRuntime& runtime, WifiState wifiState) {
    // String exerciseName = exercise ? String(exercise->name.c_str()) : String("Keine Uebung gewählt");
    
    String topText = "";
    if(runtime.phase == RepState::PRE){
        // Vorbereitungsphase
        topText = "Get Ready";
    } else if (runtime.phase == RepState::IN_PROGRESS){
        // Übungsphase
        topText = "Exercise!";
    } else if (runtime.phase == RepState::POST){
        // Nachbereitungsphase
        topText = "Rest Time :)";
    } else if (runtime.phase == RepState::SET_PAUSE){
        // Satzpause
        topText = "Set Pause";
    }
    const char* wifiText = (wifiState == WifiState::ACTIVE) ? "WiFi: Aktiv" : "WiFi: Inaktiv";
    const char* pauseText = runtime.paused ? " PAUSED" : "";
    String setRepPercentText = "Set " + String(runtime.setIndex + 1) + " Rep " + String(runtime.repIndex + 1) + " " + String(exercise->sets[runtime.setIndex].percentMaxIntensity) + "%";

    unsigned long totalSeconds = timeMillis / 1000;
    unsigned long minutes = totalSeconds / 60;
    unsigned long seconds = totalSeconds % 60;
    unsigned long decimals = (timeMillis % 1000) / 10;
    char timeString[9];
    sprintf(timeString, "%02lu:%02lu:%02lu", minutes, seconds, decimals);
    
    
    configureLine(0, 10, u8g2_font_crox1tb_tf ); // 9px Schrift
    configureLine(1, 20, u8g2_font_crox1tb_tf ); // 9px Schrift
    configureLine(2, 40, u8g2_font_crox5tb_tf); // 16px Schrift
    configureLine(3, 50, u8g2_font_crox1tb_tf ); // 9px Schrift    
    configureLine(4, 64, u8g2_font_crox1tb_tf ); // 9px Schrift

    setLine(0, topText, false);
    setLine(1, "mm  :   ss  :   cc", false);
    setLine(2, String(timeString), false);
    setLine(3, setRepPercentText, false);
    setLine(4, pauseText + String("             ") + wifiText, false);
    refresh();
}

void DisplayService::configureLine(uint8_t line, uint8_t baseline, const uint8_t* font) {
    if (line >= lines_.size()) {
        return;
    }
    lines_[line].baseline = baseline;
    lines_[line].font = font ? font : kDefaultFont;
    dirty_ = true;
}

void DisplayService::setLine(uint8_t line, const String& text, bool autoRefresh) {
    setLineInternal(line, text);
    if (autoRefresh) {
        render();
    }
}

void DisplayService::setLine(uint8_t line, const String& text, const uint8_t* font, uint8_t baseline, bool autoRefresh) {
    setLineInternal(line, text, font ? font : kDefaultFont, baseline);
    if (autoRefresh) {
        render();
    }
}

void DisplayService::printText(uint8_t lineFrom, uint8_t lineTo, const String& text) {
    if (lineFrom > lineTo) {
        std::swap(lineFrom, lineTo);
    }
    if (lineFrom >= lines_.size()) {
        return;
    }
    lineTo = std::min<uint8_t>(lineTo, static_cast<uint8_t>(lines_.size() - 1));
    for (uint8_t line = lineFrom; line <= lineTo; ++line) {
        setLineInternal(line, text);
    }
    render();
}

void DisplayService::refresh() {
    render();
}

void DisplayService::clearLines() {
    for (auto& line : lines_) {
        line.text = "";
        line.visible = false;
    }
    dirty_ = true;
}

void DisplayService::resetLayout() {
    // Default four lines roughly evenly spaced.
    const std::array<uint8_t, kDefaultLineCount> defaults{14, 28, 42, 56, 64};
    for (size_t i = 0; i < lines_.size(); ++i) {
        lines_[i].baseline = defaults[i];
        lines_[i].font = kDefaultFont;
    }
}

void DisplayService::setLineInternal(uint8_t line, const String& text) {
    if (line >= lines_.size()) {
        return;
    }
    lines_[line].text = text;
    lines_[line].visible = !text.isEmpty();
    dirty_ = true;
}

void DisplayService::setLineInternal(uint8_t line, const String& text, const uint8_t* font, uint8_t baseline) {
    if (line >= lines_.size()) {
        return;
    }
    lines_[line].text = text;
    lines_[line].font = font ? font : kDefaultFont;
    lines_[line].baseline = baseline;
    lines_[line].visible = !text.isEmpty();
    dirty_ = true;
}

void DisplayService::render() {
    if (!dirty_) {
        return;
    }

    display_.clearBuffer();
    for (const auto& line : lines_) {
        if (line.visible) {
            display_.setFont(line.font ? line.font : kDefaultFont);
            display_.drawStr(0, line.baseline, line.text.c_str());
        }
    }

    display_.sendBuffer();
    dirty_ = false;
}
