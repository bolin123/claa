#ifndef S78S_H
#define S78S_H

#include "Hal.h"

bool S78SIsOnline(void);
void S78SDataSend(char *data);
void S78SInitialize(void);
void S78SPoll(void);

#endif

