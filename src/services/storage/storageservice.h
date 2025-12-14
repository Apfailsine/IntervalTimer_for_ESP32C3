#pragma once

#include <Arduino.h>
#include <array>
#include <vector>
#include "models/datastructures.h"

class StorageService {
public:
    using ExerciseId = std::array<uint8_t, 16>;

    static constexpr size_t kMaxSets = 15;
    static constexpr size_t kMaxRepsPerSet = 30;
    static constexpr size_t kMaxExerciseNameLength = 64;
    static constexpr size_t kMaxSetLabelLength = 64;

    struct ExerciseRecord {
        ExerciseId id;
        Exercise exercise;
    };

    StorageService();

    bool addExercise(const Exercise& exercise, ExerciseId* outId = nullptr);
    bool updateExercise(const ExerciseId& id, const Exercise& exercise);
    bool removeExercise(const ExerciseId& id);
    void clear();

    bool loadPersistent();
    bool savePersistent() const;

    Exercise* findExercise(const ExerciseId& id);
    const Exercise* findExercise(const ExerciseId& id) const;

    ExerciseRecord* findRecord(const ExerciseId& id);
    const ExerciseRecord* findRecord(const ExerciseId& id) const;

    static ExerciseId fromHex(const String& hex);
    static String toHex(const ExerciseId& id);

    const std::vector<ExerciseRecord>& exercises() const { return exercises_; }

private:
    ExerciseId generateId();
    bool idExists(const ExerciseId& id) const;
    bool validateExercise(const Exercise& exercise) const;
    bool serialize(std::vector<uint8_t>& buffer) const;
    bool deserialize(const uint8_t* data, size_t length);

    std::vector<ExerciseRecord> exercises_;
};
