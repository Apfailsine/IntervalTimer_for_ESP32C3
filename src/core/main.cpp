#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include "datastructures.h"
#include "board.h"

#define BUTTON_PIN 0 // GPIO-Pin für den Button (z. B. GPIO 0)


// Zugangsdaten für den Access Point
const char* ssid = "ESP32_IntervalTimer";
const char* password = "12345678";

// Webserver auf Port 80
WebServer server(80);

unsigned long timeStart = 0.0;
ExerciseState E = ExerciseState::IDLE;

U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// HTML-Seite
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Interval Timer</title>
</head>
<body>
    <h1>Interval Timer Configuration</h1>
    <form action="/submit" method="GET">
        <label for="exercise">Exercise Name:</label><br>
        <input type="text" id="exercise" name="exercise"><br><br>
        <label for="sets">Number of Sets:</label><br>
        <input type="number" id="sets" name="sets"><br><br>
        <label for="reps">Number of Reps per Set:</label><br>
        <input type="number" id="reps" name="reps"><br><br>
        <input type="submit" value="Submit">
    </form>
</body>
</html>
)rawliteral";

// Funktion: HTML-Seite bereitstellen
void handleRoot() {
    server.send(200, "text/html", htmlPage);
}

void setup() {
    Serial.begin(115200);

    // Webserver-Task starten
    xTaskCreate(
        webServerTask,   // Funktion
        "WebServerTask", // Name des Tasks
        8192,            // Stack-Größe
        NULL,            // Parameter
        1,               // Priorität
        NULL             // Task-Handle
    );

    // Timer-Task starten
    xTaskCreate(
        timerTask,       // Funktion
        "TimerTask",     // Name des Tasks
        2048,            // Stack-Größe
        NULL,            // Parameter
        1,               // Priorität
        NULL             // Task-Handle
    );

    // Button-Task starten
    xTaskCreate(
        buttonTask,       // Funktion
        "ButtonTask",     // Name des Tasks
        2048,             // Stack-Größe
        NULL,             // Parameter
        1,                // Priorität
        NULL              // Task-Handle
    );

    display.begin();
    unsigned long currentMillis = millis();
    timeStart = currentMillis;
}

void timerTask(void* parameter) {


    // ToDO: Input controls via wifi oder BLE implementieren
    // Setzt numSets, timeRep, timeRest, timeStart und unterbrochen entsprechend der Eingaben
    // Sowie die States STARTED, STOPPED, PAUSED, IDLE verwalten

    if(E == ExerciseState::STARTED) {

        exercise();

    } else if(E == ExerciseState::PAUSED) {

        // Übung pausiert, nichts tun

    } else if(E == ExerciseState::STOPPED) {

        // Übung gestoppt, alles zurücksetzen
        displayTime(0);
        // Reset aller Variablen und anzeige Stopped
        E = ExerciseState::IDLE;

    } else if(E == ExerciseState::IDLE) {

        // Warte auf Startbefehl
        timeStart = millis();
        display.clearBuffer();                 // Puffer löschen
        display.setFont(u8g2_font_ncenB08_tr); // Schriftart setzen
        display.drawStr(0, 10, "Hello World!"); // Text zeichnen
        display.sendBuffer();                  // Pufferinhalt anzeigen
        delay(1000);    
    }
}


// Funktion: Formulardaten verarbeiten
void handleSubmit() {
    String exercise = server.arg("exercise");
    int sets = server.arg("sets").toInt();
    int reps = server.arg("reps").toInt();

    // Daten ausgeben (später kannst du sie speichern oder verarbeiten)
    Serial.println("Exercise: " + exercise);
    Serial.println("Sets: " + String(sets));
    Serial.println("Reps: " + String(reps));

    // Bestätigungsseite anzeigen
    server.send(200, "text/html", "<h1>Configuration Saved!</h1>");
}

// Webserver-Task
void webServerTask(void* parameter) {
    // Access Point starten
    WiFi.softAP(ssid, password);
    Serial.println("Access Point gestartet!");
    Serial.print("IP-Adresse: ");
    Serial.println(WiFi.softAPIP());

    // Routen für den Webserver definieren
    server.on("/", handleRoot);       // HTML-Seite
    server.on("/submit", handleSubmit); // Formulardaten

    // Webserver starten
    server.begin();
    Serial.println("Webserver gestartet!");

    // Webserver-Loop
    while (true) {
        server.handleClient(); // Anfragen bearbeiten
        vTaskDelay(10 / portTICK_PERIOD_MS); // Task kurz pausieren
    }
}

void exercise() {
    




    unsigned long now = millis();
}

void displayTime(unsigned long millisecs) {
    // Zeigt die Zeit in min:sec:decimal format an
    unsigned long totalSeconds = millisecs / 1000;
    unsigned long minutes = totalSeconds / 60;
    unsigned long seconds = totalSeconds % 60;
    unsigned long decimals = (millisecs % 1000) / 10;
    char timeString[9];
    sprintf(timeString, "%02lu:%02lu:%02lu", minutes, seconds, decimals);
    display.clearBuffer();
    display.setFont(u8g2_font_ncenB08_tr);
    display.drawStr(0, 30, timeString);
    display.sendBuffer();

}

/*
Exercise pushUps("Push-Ups");

    // Set 1 erstellen und hinzufügen
    Set set1(60, 80);
    set1.reps.push_back(Rep(30, 10));
    set1.reps.push_back(Rep(25, 15));
    pushUps.sets.push_back(set1);

    // Set 2 erstellen und hinzufügen
    Set set2(90, 70);
    set2.reps.push_back(Rep(20, 10));
    set2.reps.push_back(Rep(15, 20));
    pushUps.sets.push_back(set2);

*/

void loop() {
    // put your main code here, to run repeatedly:
}



void buttonTask(void* parameter) {
    

    BoardService
}