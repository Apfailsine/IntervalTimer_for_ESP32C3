#ifndef GLOBALS_H
#define GLOBALS_H

// core
#include <Arduino.h>
#include "models/datastructures.h"
#include "services/board/board.h"
#include "services/display/displayservice.h"
#include "services/storage/storageservice.h"
#include "services/web/webpage.h"


//+  ######################     HARDWARE SETUP      ########################
// TODO: consider removing sensorType

//*  ####################     END HARDWARE SETUP      ######################


extern BoardService boardService;
extern DisplayService displayService;
extern StorageService storageService;
extern WebService webService;

extern unsigned long timePrev;
extern unsigned long timePauseStart;
extern unsigned long now;
extern unsigned long timeRep;

#endif // GLOBALS_H
