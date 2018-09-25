#include "DS18B20.h"
#include "S78S.h"
#include "Hal.h"

static void testReadTemp(void)
{
    static uint32_t lastTime;
    uint8_t temp;
    char buff[16] = "";

    if(HalTime() - lastTime > 10000) //10 secends
    {
        temp = DS18B20ReadTemp();
        if(S78SIsOnline())
        {
            sprintf(buff, "Temp = %d", temp);
            S78SDataSend(buff);
        }
        lastTime = HalTime();
    }
}

void APPPacketRecv(char *pack)
{
}

void APPInitialize(void)
{
    HalInitialize();
    S78SInitialize();
    
}

void APPPoll(void)
{
    S78SPoll();
    HalPoll();
    testReadTemp();
}

