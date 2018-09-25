#ifndef HAL_H
#define HAL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "stm8l15x.h"

const char *HalGetAppkey(void);
const char *HalGetAppeui(void);
const char *HalGetDeveui(void);
void HalWaitUs(uint16_t tenUs);
void HalReboot(void);
uint32_t HalTime(void);
void HalInterruptSet(bool enable);
void HalUartWrite(uint8_t *data, uint16_t len);
void HalInitialize(void);
void HalPoll(void);
#endif
