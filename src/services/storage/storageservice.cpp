#include "storageservice.h"

#include <algorithm>
#include <cstdio>
#include <vector>
#include <cstring>
#include <utility>

#ifdef ESP_PLATFORM
#include <esp_random.h>
#include <Preferences.h>
#else
#include <cstdlib>
#include <ctime>
#endif

namespace {
constexpr const char* kPrefsNamespace = "interval";
constexpr const char* kPrefsKey = "exercises";
constexpr uint16_t kStorageVersion = 1;

void appendUint16(std::vector<uint8_t>& buffer, uint16_t value) {
    buffer.push_back(static_cast<uint8_t>(value & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
}

void appendUint32(std::vector<uint8_t>& buffer, uint32_t value) {
    buffer.push_back(static_cast<uint8_t>(value & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
}

bool readUint16(const uint8_t* data, size_t length, size_t& offset, uint16_t& outValue) {
    if (offset + 2 > length) {
        return false;
    }
    outValue = static_cast<uint16_t>(data[offset]) |
               (static_cast<uint16_t>(data[offset + 1]) << 8);
    offset += 2;
    return true;
}

bool readUint32(const uint8_t* data, size_t length, size_t& offset, uint32_t& outValue) {
    if (offset + 4 > length) {
        return false;
    }
    outValue = static_cast<uint32_t>(data[offset]) |
               (static_cast<uint32_t>(data[offset + 1]) << 8) |
               (static_cast<uint32_t>(data[offset + 2]) << 16) |
               (static_cast<uint32_t>(data[offset + 3]) << 24);
    offset += 4;
    return true;
}

bool readBytes(const uint8_t* data, size_t length, size_t& offset, uint8_t* target, size_t count) {
    if (offset + count > length) {
        return false;
    }
    std::memcpy(target, data + offset, count);
    offset += count;
    return true;
}
} // namespace

StorageService::StorageService() {
#ifndef ESP_PLATFORM
    static bool seeded = false;
    if (!seeded) {
        std::srand(static_cast<unsigned>(std::time(nullptr)));
        seeded = true;
    }
#endif
}

StorageService::ExerciseId StorageService::generateId() {
    ExerciseId id{};
    size_t offset = 0;
    while (offset < id.size()) {
#ifdef ESP_PLATFORM
        uint32_t randomValue = esp_random();
#else
        uint32_t randomValue = static_cast<uint32_t>(std::rand());
#endif
        for (int byteIndex = 0; byteIndex < 4 && offset < id.size(); ++byteIndex) {
            id[offset++] = static_cast<uint8_t>((randomValue >> (byteIndex * 8)) & 0xFF);
        }
    }
    return id;
}

bool StorageService::idExists(const ExerciseId& id) const {
    return std::any_of(exercises_.begin(), exercises_.end(), [&](const ExerciseRecord& record) {
        return record.id == id;
    });
}

bool StorageService::validateExercise(const Exercise& exercise) const {
    if (exercise.sets.size() > kMaxSets) {
        Serial.println("[Storage] Too many sets for exercise.");
        return false;
    }
    if (exercise.name.size() > kMaxExerciseNameLength) {
        Serial.println("[Storage] Exercise name exceeds length limit.");
        return false;
    }
    for (const auto& set : exercise.sets) {
        if (set.reps.size() > kMaxRepsPerSet) {
            Serial.println("[Storage] Too many reps in set.");
            return false;
        }
        if (set.label.size() > kMaxSetLabelLength) {
            Serial.println("[Storage] Set label exceeds length limit.");
            return false;
        }
    }
    return true;
}

bool StorageService::addExercise(const Exercise& exercise, ExerciseId* outId) {
    if (!validateExercise(exercise)) {
        return false;
    }

    ExerciseRecord record{};
    record.exercise = exercise;
    record.id = generateId();
    while (idExists(record.id)) {
        record.id = generateId();
    }

    exercises_.push_back(record);
    if (outId) {
        *outId = record.id;
    }
    return true;
}

bool StorageService::updateExercise(const ExerciseId& id, const Exercise& exercise) {
    auto it = std::find_if(exercises_.begin(), exercises_.end(), [&](const ExerciseRecord& record) {
        return record.id == id;
    });
    if (it == exercises_.end()) {
        return false;
    }
    if (!validateExercise(exercise)) {
        return false;
    }
    it->exercise = exercise;
    return true;
}

bool StorageService::removeExercise(const ExerciseId& id) {
    auto it = std::find_if(exercises_.begin(), exercises_.end(), [&](const ExerciseRecord& record) {
        return record.id == id;
    });
    if (it == exercises_.end()) {
        return false;
    }
    exercises_.erase(it);
    return true;
}

void StorageService::clear() {
    exercises_.clear();
}

Exercise* StorageService::findExercise(const ExerciseId& id) {
    auto it = std::find_if(exercises_.begin(), exercises_.end(), [&](const ExerciseRecord& record) {
        return record.id == id;
    });
    return it != exercises_.end() ? &it->exercise : nullptr;
}

const Exercise* StorageService::findExercise(const ExerciseId& id) const {
    auto it = std::find_if(exercises_.begin(), exercises_.end(), [&](const ExerciseRecord& record) {
        return record.id == id;
    });
    return it != exercises_.end() ? &it->exercise : nullptr;
}

StorageService::ExerciseRecord* StorageService::findRecord(const ExerciseId& id) {
    auto it = std::find_if(exercises_.begin(), exercises_.end(), [&](const ExerciseRecord& record) {
        return record.id == id;
    });
    return it != exercises_.end() ? &(*it) : nullptr;
}

const StorageService::ExerciseRecord* StorageService::findRecord(const ExerciseId& id) const {
    auto it = std::find_if(exercises_.begin(), exercises_.end(), [&](const ExerciseRecord& record) {
        return record.id == id;
    });
    return it != exercises_.end() ? &(*it) : nullptr;
}

bool StorageService::serialize(std::vector<uint8_t>& buffer) const {
    buffer.clear();
    buffer.reserve(256);

    appendUint16(buffer, kStorageVersion);
    appendUint16(buffer, static_cast<uint16_t>(exercises_.size()));

    for (const auto& record : exercises_) {
        buffer.insert(buffer.end(), record.id.begin(), record.id.end());

        const std::string& name = record.exercise.name;
        appendUint16(buffer, static_cast<uint16_t>(name.size()));
        buffer.insert(buffer.end(), name.begin(), name.end());

        appendUint16(buffer, static_cast<uint16_t>(record.exercise.sets.size()));
        for (const auto& set : record.exercise.sets) {
            appendUint16(buffer, static_cast<uint16_t>(set.label.size()));
            buffer.insert(buffer.end(), set.label.begin(), set.label.end());

            appendUint32(buffer, static_cast<uint32_t>(set.timePauseAfter));
            appendUint32(buffer, static_cast<uint32_t>(set.percentMaxIntensity));

            appendUint16(buffer, static_cast<uint16_t>(set.reps.size()));
            for (const auto& rep : set.reps) {
                appendUint32(buffer, static_cast<uint32_t>(rep.timeRep));
                appendUint32(buffer, static_cast<uint32_t>(rep.timeRest));
            }
        }
    }
    return true;
}

bool StorageService::deserialize(const uint8_t* data, size_t length) {
    exercises_.clear();
    if (length == 0) {
        return true;
    }

    size_t offset = 0;
    uint16_t version = 0;
    if (!readUint16(data, length, offset, version)) {
        return false;
    }
    if (version != kStorageVersion) {
        Serial.println("[Storage] Incompatible storage version.");
        return false;
    }

    uint16_t count = 0;
    if (!readUint16(data, length, offset, count)) {
        return false;
    }

    exercises_.reserve(count);

    for (uint16_t recordIndex = 0; recordIndex < count; ++recordIndex) {
        ExerciseRecord record{};
        if (!readBytes(data, length, offset, record.id.data(), record.id.size())) {
            return false;
        }

        uint16_t nameLength = 0;
        if (!readUint16(data, length, offset, nameLength)) {
            return false;
        }
        if (nameLength > kMaxExerciseNameLength) {
            return false;
        }
        record.exercise.name.clear();
        if (nameLength > 0) {
            record.exercise.name.resize(nameLength);
            if (!readBytes(data, length, offset, reinterpret_cast<uint8_t*>(&record.exercise.name[0]), nameLength)) {
                return false;
            }
        }

        uint16_t setCount = 0;
        if (!readUint16(data, length, offset, setCount)) {
            return false;
        }
        if (setCount > kMaxSets) {
            return false;
        }
        record.exercise.sets.reserve(setCount);

        for (uint16_t setIndex = 0; setIndex < setCount; ++setIndex) {
            Set set;

            uint16_t labelLength = 0;
            if (!readUint16(data, length, offset, labelLength)) {
                return false;
            }
            if (labelLength > kMaxSetLabelLength) {
                return false;
            }
            set.label.clear();
            if (labelLength > 0) {
                set.label.resize(labelLength);
                if (!readBytes(data, length, offset, reinterpret_cast<uint8_t*>(&set.label[0]), labelLength)) {
                    return false;
                }
            }

            uint32_t pauseAfter = 0;
            if (!readUint32(data, length, offset, pauseAfter)) {
                return false;
            }
            set.timePauseAfter = static_cast<int>(pauseAfter);

            uint32_t percent = 0;
            if (!readUint32(data, length, offset, percent)) {
                return false;
            }
            set.percentMaxIntensity = static_cast<int>(percent);

            uint16_t repCount = 0;
            if (!readUint16(data, length, offset, repCount)) {
                return false;
            }
            if (repCount > kMaxRepsPerSet) {
                return false;
            }
            set.reps.reserve(repCount);

            for (uint16_t repIndex = 0; repIndex < repCount; ++repIndex) {
                uint32_t repTime = 0;
                uint32_t restTime = 0;
                if (!readUint32(data, length, offset, repTime) ||
                    !readUint32(data, length, offset, restTime)) {
                    return false;
                }
                set.reps.emplace_back(static_cast<int>(repTime), static_cast<int>(restTime));
            }

            record.exercise.sets.push_back(std::move(set));
        }

        exercises_.push_back(std::move(record));
    }

    return true;
}

bool StorageService::loadPersistent() {
#ifdef ESP_PLATFORM
    Preferences prefs;
    if (!prefs.begin(kPrefsNamespace, true)) {
        Serial.println("[Storage] Failed to open preferences for reading.");
        return false;
    }

    size_t length = prefs.getBytesLength(kPrefsKey);
    std::vector<uint8_t> buffer(length > 0 ? length : 0);
    if (length > 0) {
        prefs.getBytes(kPrefsKey, buffer.data(), length);
    }
    prefs.end();

    if (!deserialize(buffer.data(), buffer.size())) {
        Serial.println("[Storage] Failed to deserialize exercises.");
        exercises_.clear();
        return false;
    }

    Serial.printf("[Storage] Loaded %u exercises from NVS.\n", static_cast<unsigned>(exercises_.size()));
    return true;
#else
    Serial.println("[Storage] Persistent storage not available on this platform.");
    return true;
#endif
}

bool StorageService::savePersistent() const {
#ifdef ESP_PLATFORM
    std::vector<uint8_t> buffer;
    if (!serialize(buffer)) {
        Serial.println("[Storage] Serialization failed.");
        return false;
    }

    Preferences prefs;
    if (!prefs.begin(kPrefsNamespace, false)) {
        Serial.println("[Storage] Failed to open preferences for writing.");
        return false;
    }

    bool ok = true;
    if (buffer.empty()) {
        ok = prefs.remove(kPrefsKey);
    } else {
        size_t written = prefs.putBytes(kPrefsKey, buffer.data(), buffer.size());
        ok = (written == buffer.size());
    }
    prefs.end();

    if (!ok) {
        Serial.println("[Storage] Failed to persist exercises.");
        return false;
    }

    Serial.printf("[Storage] Saved %u exercises to NVS.\n", static_cast<unsigned>(exercises_.size()));
    return true;
#else
    Serial.println("[Storage] Skipping persistence on this platform.");
    return true;
#endif
}

StorageService::ExerciseId StorageService::fromHex(const String& hex) {
    ExerciseId id{};
    if (hex.length() != id.size() * 2) {
        return id;
    }

    auto nibble = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
        if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
        return -1;
    };

    for (size_t i = 0; i < id.size(); ++i) {
        int highVal = nibble(hex[i * 2]);
        int lowVal = nibble(hex[i * 2 + 1]);
        if (highVal < 0 || lowVal < 0) {
            id.fill(0);
            return id;
        }
        id[i] = static_cast<uint8_t>((highVal << 4) | lowVal);
    }
    return id;
}

String StorageService::toHex(const ExerciseId& id) {
    char buffer[id.size() * 2 + 1];
    for (size_t i = 0; i < id.size(); ++i) {
        std::snprintf(&buffer[i * 2], 3, "%02X", id[i]);
    }
    buffer[id.size() * 2] = '\0';
    return String(buffer);
}
