#include "globals.h"

BoardService boardService;
DisplayService displayService;
StorageService storageService;
WebService webService;

unsigned long timePrev = 0;
unsigned long timePauseStart = 0;
unsigned long now = 0;
unsigned long timeRep = 0;
