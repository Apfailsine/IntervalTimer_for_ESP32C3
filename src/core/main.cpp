#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include "models/datastructures.h"
#include "globals.h"

#define BUTTON_PIN 0 // GPIO-Pin für den Button (z. B. GPIO 0)

// declaration of functions
void printTimer(unsigned long timeMillis, String label = "");
void resumeExercise(unsigned long now);
void doExerciseStep(const Exercise& exercise, unsigned long now);
static unsigned long elapsedMs(unsigned long now, unsigned long since);
void pauseExercise(unsigned long now);

// Zugangsdaten für den Access Point
const char* ssid = "ESP32_IntervalTimer";
const char* password = "12345678";

// Webserver auf Port 80
WebServer server(80);

unsigned long timeStart = 0.0;
ExerciseState E = ExerciseState::IDLE;
WifiState W = WifiState::INACTIVE;
RepState R = RepState::PRE;
static ExerciseRuntime runtime;

namespace {
StorageService::ExerciseId g_selectedExerciseId{};
bool g_hasSelectedExercise = false;

void logSelectedExercise(const StorageService::ExerciseRecord& record) {
    Serial.printf("[Button] Selected exercise: %s (%u sets)\n",
                  record.exercise.name.c_str(),
                  static_cast<unsigned>(record.exercise.sets.size()));
    for (size_t setIndex = 0; setIndex < record.exercise.sets.size(); ++setIndex) {
        const Set& set = record.exercise.sets[setIndex];
        Serial.printf("  Set %u (%s): intensity=%d%%, reps=%u\n",
                      static_cast<unsigned>(setIndex + 1),
                      set.label.c_str(),
                      set.percentMaxIntensity,
                      static_cast<unsigned>(set.reps.size()));
    }
}
} // namespace

void timerTask(void* parameter) {
    for (;;) {
        
        // Setzt numSets, timeRep, timeRest, timeStart und unterbrochen entsprechend der Eingaben
        // Sowie die States STARTED, STOPPED, PAUSED, IDLE verwalten
        
        now = millis();

        if (E == ExerciseState::STARTED) {
            // exercise();
            resumeExercise(now);
            if (g_hasSelectedExercise) {
                if (const Exercise* exercise = storageService.findExercise(g_selectedExerciseId)) {
                    // Serial.printf("[TimerTask] Übung läuft: %s\n", exercise->name.c_str());
                    // Hier die Logik zum Verarbeiten der Übung implementieren
                    doExerciseStep(*exercise, now);
                } else {
                    Serial.println("[TimerTask] Ausgewählte Übung nicht mehr verfügbar.");
                    g_hasSelectedExercise = false;
                    E = ExerciseState::IDLE;
                    displayService.showStatus("Keine Übung", "Bitte auswählen");
                }
            } else {
                Serial.println("[TimerTask] Keine Übung ausgewählt.");
                E = ExerciseState::IDLE;
                displayService.showStatus("Bereit", "Button drücken");
            }

        } else if (E == ExerciseState::PAUSED) {
            // Übung pausiert, nichts tun
            pauseExercise(now);
            LOG_COLOR_D("TimerTask: PAUSED state - exercise is paused.\n");
            displayService.setLine(1, "PAUSED", u8g2_font_ncenB24_tr, 48);
            displayService.refresh();

        } else if (E == ExerciseState::STOPPED) {
            // Übung gestoppt, alles zurücksetzen
            // displayTime(0);
            // Reset aller Variablen und anzeige Stopped
            runtime.active = false;
            runtime.paused = false;
            E = ExerciseState::IDLE;
            displayService.showStatus("Gestoppt", "Button: Start");

        } else if (E == ExerciseState::IDLE) {
            LOG_COLOR_D("TimerTask: IDLE state - waiting for start command.\n");
            // Warte auf Startbefehl
            timePrev = millis();
            displayService.chooseExercise(storageService.findExercise(g_selectedExerciseId), W);
        }

        // if (const Exercise* last = webService.lastExercise()) {
        //     Serial.printf("[TimerTask] Last exercise: %s, sets=%u\n",
        //                   last->name.c_str(),
        //                   static_cast<unsigned>(last->sets.size()));
        //     for (size_t i = 0; i < last->sets.size(); ++i) {
        //         const Set& set = last->sets[i];
        //         Serial.printf("  Set %u (%s): intensity=%d%%, reps=%u\n",
        //                       static_cast<unsigned>(i + 1),
        //                       set.label.c_str(),
        //                       set.percentMaxIntensity,
        //                       static_cast<unsigned>(set.reps.size()));
        //     }
        // } else {
        //     Serial.println("[TimerTask] No exercise stored yet.");
        // }

        vTaskDelay(25 / portTICK_PERIOD_MS);
    }
}
// Webserver-Task
void webServerTask(void* parameter) {
    bool apActive = false;

    for (;;) {
        if (W == WifiState::ACTIVE) {
            if (!apActive) {

                WiFi.mode(WIFI_AP);
                WiFi.softAP(ssid, password);
                Serial.println("Access Point gestartet!");
                Serial.print("IP-Adresse: ");
                Serial.println(WiFi.softAPIP());

                webService.registerRoutes(server);
                server.begin();
                Serial.println("Webserver gestartet!");
                apActive = true;
            }

            server.handleClient();
            vTaskDelay(10 / portTICK_PERIOD_MS);
        } else {
            if (apActive) {
                server.stop();
                WiFi.softAPdisconnect(true);
                WiFi.mode(WIFI_OFF);
                Serial.println("Access Point gestoppt.");
                apActive = false;
            }

            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
    }
}

void loop() {
    // put your main code here, to run repeatedly:
}

void buttonTask(void* parameter) {
    static size_t currentExerciseIndex = 0;
    for (;;) {

        const auto *buttons = boardService.getButtons();
        // handleButtonLogic(buttons); 


        // TODO: Button handling und State-Wechsel implementieren
        // load first exercise from storage
        
        const auto& records = storageService.exercises();

        
        // bei short press:
        switch(*buttons)
        {
        case ButtonState::SHORT_PRESS:
           if (E == ExerciseState::IDLE){
                if (!records.empty()) {
                    const size_t safeIndex = currentExerciseIndex % records.size();
                    const auto& record = records[safeIndex];
                    currentExerciseIndex = (safeIndex + 1) % records.size();
                    g_selectedExerciseId = record.id;
                    g_hasSelectedExercise = true;
                    logSelectedExercise(record);
                    displayService.showStatus("Übung", record.exercise.name.c_str());
                } else {
                    Serial.println("[Button] Keine gespeicherten Übungen vorhanden.");
                    // displayService.showStatus("Keine Übungen", "Web anlegen");
                }
           }
           else if(E == ExerciseState::PAUSED){
                Serial.println("[Button] Setze Übung fort.");
                E = ExerciseState::STARTED;
           } else if(E == ExerciseState::STARTED){
                Serial.println("[Button] Pausiere Übung.");
                E = ExerciseState::PAUSED;
           }
            break;
        // bei long press:
        case ButtonState::LONG_PRESS:
           if (E == ExerciseState::IDLE){
                if (g_hasSelectedExercise) {
                    if (const Exercise* exercise = storageService.findExercise(g_selectedExerciseId)) {
                        Serial.printf("[Button] Starte Übung: %s\n", exercise->name.c_str());
                        // Hier muss die Übung Starten oder ein Flag gesetzt werden
                        E = ExerciseState::STARTED;
                        // Kann ich außerhalb auf den recied zugreifen?
                        runtime.active = true;
                        runtime.paused = false;
                        runtime.phaseStart = millis();
                        // displayService.showStatus("Start", exercise->name.c_str());
                    } else {
                        Serial.println("[Button] Ausgewählte Übung nicht mehr verfügbar.");
                        g_hasSelectedExercise = false;
                        // displayService.showStatus("Übung fehlt", "Bitte wählen");
                    }
                }
           } else {
               if (const Exercise* exercise = storageService.findExercise(g_selectedExerciseId)){
                   Serial.printf("[Button] Stoppe Übung: %s\n", exercise->name.c_str());
                    // displayService.showStatus("Gestoppt", exercise->name.c_str());
                }
                g_hasSelectedExercise = false;
                E = ExerciseState::STOPPED;
           }
            break;
        case ButtonState::EXTRA_LONG_PRESS:
            // currently not used
            W = (W == WifiState::INACTIVE) ? WifiState::ACTIVE : WifiState::INACTIVE;
            Serial.printf("[Button] WiFi %s\n", W == WifiState::ACTIVE ? "aktiv" : "inaktiv");
            break;
        case ButtonState::NO_PRESS:
        default:
            // Do nothing if no press
            break;
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("Booting IntervalTimer...");

    displayService.begin();
    // displayService.showBootScreen();

    if (!storageService.loadPersistent()) {
        Serial.println("[Setup] Konnte gespeicherte Übungen nicht laden.");
        displayService.showStatus("Speicher", "Keine Daten");
    }

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

    unsigned long currentMillis = millis();
    timeStart = currentMillis;
}

void handleButtonLogic(const ButtonState *buttons)
{

    switch ((*buttons)) // Check the state of the button
    {
    case ButtonState::SHORT_PRESS:
        Serial.println("main - handleButtonLogic Button short-press");
        
        break;

    case ButtonState::LONG_PRESS:
        Serial.println("main - handleButtonLogic Button long-press");
        break;

    case ButtonState::EXTRA_LONG_PRESS:
        Serial.println("main - handleButtonLogic Button extra-long-press");
        
        break;

    case ButtonState::NO_PRESS:
    default:
        // Do nothing if no press
        break;
    }
    
}

void printTimer(unsigned long timeMillis, String label)
{
    unsigned long totalSeconds = timeMillis / 1000;
    unsigned long minutes = totalSeconds / 60;
    unsigned long seconds = totalSeconds % 60;
    unsigned long decimals = (timeMillis % 1000) / 10;
    char timeString[9];
    sprintf(timeString, "%02lu:%02lu:%02lu", minutes, seconds, decimals);
    Serial.printf("Timer %s: %s\n", label.c_str(), timeString);
    const char* headline = label.length() > 0 ? label.c_str() : "Timer";
    // displayService.showCountdown(headline, timeMillis);
}
    
void resumeExercise(unsigned long now) {
    if (!runtime.active || !runtime.paused) return;
    runtime.paused = false;
    runtime.phaseStart = now - runtime.pauseOffset;
    runtime.pauseOffset = 0;
}

void doExerciseStep(const Exercise& exercise, unsigned long now) {
    if (!runtime.active || runtime.paused || runtime.setIndex >= exercise.sets.size()) {
        return;
    }

    const Set& set = exercise.sets[runtime.setIndex];
    const Rep& rep = set.reps[runtime.repIndex];
    unsigned long elapsed = now - runtime.phaseStart;

    switch (runtime.phase) {
    case RepState::PRE:
        if (elapsed >= 3000UL) {
            runtime.phase = RepState::IN_PROGRESS;
            runtime.phaseStart = now;
        } else {
            // display "Get Ready" with countdown
            printTimer(3000UL - elapsed, "Get Ready");
            displayService.playTimer(&exercise, 3000UL - elapsed, runtime, W);
            
        }
        break;
    case RepState::IN_PROGRESS:
        if (elapsed >= rep.timeRep * 1000UL) {
            runtime.phase = RepState::POST;
            runtime.phaseStart = now;
        } else {
            // display "Exercise" with countdown and intensity
            printTimer(rep.timeRep * 1000UL - elapsed, "Exercise");
            displayService.playTimer(&exercise, rep.timeRep * 1000UL - elapsed, runtime, W);
        }
        break;
    case RepState::POST:
        if (elapsed >= rep.timeRest * 1000UL) {
            // zur nächsten Rep (ggf. Setwechsel, Pause nach Set usw.)
            if (++runtime.repIndex < set.reps.size()) {
                runtime.phase = RepState::PRE;
                runtime.phaseStart = now;
            } else {
                // Set erledigt → Set-Pause einleiten
                runtime.phase = RepState::SET_PAUSE;      // zusätzlicher Zustand
                runtime.phaseStart = now;
                runtime.repIndex = 0;
            }
        } else {
            printTimer(rep.timeRest * 1000UL - elapsed, "Rest");
            displayService.playTimer(&exercise, rep.timeRest * 1000UL - elapsed, runtime, W);
             // display "Rest" with countdown
        }
        break;
    case RepState::SET_PAUSE:
        if (elapsed >= set.timePauseAfter * 1000UL) {
            if (++runtime.setIndex + 1 < exercise.sets.size()) {
                runtime.phase = RepState::PRE;
                runtime.phaseStart = now;
            } else {
                // gesamte Übung fertig
                runtime.active = false;
                E = ExerciseState::STOPPED;
                Serial.println("Exercise completed. Gut gemacht! :)");
                // displayService.showStatus("Fertig!", "Gut gemacht! :)");
                delay(2000);
            }
        } else {
            printTimer(set.timePauseAfter * 1000UL - elapsed, "Set Pause");
            displayService.playTimer(&exercise, set.timePauseAfter * 1000UL - elapsed, runtime, W);
             // display "Set Pause" with countdown
        }
    break;
    }
}

static unsigned long elapsedMs(unsigned long now, unsigned long since) {
    return now - since; // bei Bedarf auf millis()-Overflow anpassen
}

void pauseExercise(unsigned long now) {
    if (!runtime.active || runtime.paused) return;
    runtime.paused = true;
    runtime.pauseOffset = elapsedMs(now, runtime.phaseStart);
}




