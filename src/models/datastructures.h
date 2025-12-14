#ifndef DATASTRUCTURES_H
#define DATASTRUCTURES_H

#include <array>
#include <vector>
#include <string>
#include <utility>

enum class ButtonState
{
    NO_PRESS,
    SHORT_PRESS,
    LONG_PRESS,
    EXTRA_LONG_PRESS,
};

enum class WifiState
{
    ACTIVE,
    INACTIVE,
};

enum class RepState
{
    PRE,
    POST,
    IN_PROGRESS,
    SET_PAUSE,
};

struct ExerciseRuntime {
    size_t setIndex = 0;
    size_t repIndex = 0;
    RepState phase = RepState::PRE;
    unsigned long phaseStart = 0;
    unsigned long pauseOffset = 0;
    bool paused = false;
    bool active = false;
};

enum class ExerciseState
{
    STARTED,
    STOPPED,
    PAUSED,
    IDLE,
};

struct Rep {
    int timeRep = 7;
    int timeRest = 30;

    Rep(int timeRep, int timeRest) : timeRep(timeRep), timeRest(timeRest) {}
};

struct Set {
    std::string label;      // Anzeigename des Sets
    std::vector<Rep> reps;  // Dynamisches Array von Wiederholungen
    int timePauseAfter = 0;
    int percentMaxIntensity = 100;

    Set() = default;
    Set(std::string label, int timePauseAfter, int percentMaxIntensity)
        : label(std::move(label)), timePauseAfter(timePauseAfter), percentMaxIntensity(percentMaxIntensity) {}
};

struct Exercise {
    std::string name;             // Name der Übung
    std::vector<Set> sets;        // Dynamisches Array von Sets

    Exercise() = default;
    explicit Exercise(const std::string& name) : name(name) {}
};

#endif // DATASTRUCTURES_H