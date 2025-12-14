#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <WebServer.h>
#include <stdint.h>

#include "models/datastructures.h"
#include "services/storage/storageservice.h"

class WebService {
public:
    WebService();

    void registerRoutes(WebServer& server);

    const Exercise* lastExercise() const;

private:
    void handleExercisesList(WebServer& server);
    void handleExerciseDetail(WebServer& server);
    void handleExerciseDelete(WebServer& server);
    void handleSubmit(WebServer& server);

    StorageService::ExerciseId lastExerciseId_;
    uint8_t hasExercise_;
};

#endif // WEBPAGE_H