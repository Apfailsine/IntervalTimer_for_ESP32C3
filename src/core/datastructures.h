#define DATASTRUCTURES_H

#include <array>
#include <vector>
#include <string>

enum class ButtonState
{
    NO_PRESS,
    SHORT_PRESS,
    LONG_PRESS,
    EXTRA_LONG_PRESS,
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
    std::vector<Rep> reps; // Dynamisches Array von Wiederholungen
    int timePause = 0;
    int percentMaxIntensity = 100;

    Set(int timePause, int percentMaxIntensity): timePause(timePause), percentMaxIntensity(percentMaxIntensity) {}
};

struct Exercise {
    std::string name;             // Name der Übung
    std::vector<Set> sets;        // Dynamisches Array von Sets

    Exercise(const std::string& name) : name(name) {}
};