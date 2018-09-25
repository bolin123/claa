#include "APP.h"

int main(void)
{
    APPInitialize();
    while(1)
    {
        APPPoll();
    }
    //return 0;
}
