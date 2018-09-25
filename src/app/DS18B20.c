#include "DS18B20.h"
#include "stm8l15x_gpio.h"

//pd0
#define OUT_DQ() GPIO_Init(GPIOD, GPIO_Pin_0, GPIO_Mode_Out_PP_Low_Fast)
#define CLR_DQ() GPIO_ResetBits(GPIOD, GPIO_Pin_0)
#define SET_DQ() GPIO_SetBits(GPIOD, GPIO_Pin_0)
#define IN_DQ() GPIO_Init(GPIOD, GPIO_Pin_0, GPIO_Mode_In_PU_No_IT)
#define GET_DQ() GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_0)


//-----复位-----
static void resetOnewire(void) 
{
    OUT_DQ();
    CLR_DQ();
    HalWaitUs(75);
    SET_DQ();
    HalWaitUs(10);
    IN_DQ(); 
    //while(GET_DQ());
    while(!(GET_DQ()));
    SET_DQ();
}

//-----读数据-----
static uint8_t rOnewire(void)
{
    uint8_t data=0,i=0;
    for(i=0;i<8;i++)
    {
        data=data>>1;
        OUT_DQ();
        CLR_DQ();
        IN_DQ();
        if(GET_DQ()) data|=0x80;
        else while(!(GET_DQ()));
        HalWaitUs(6);
    }
    return(data);
}
//-----写数据-----
static void wOnewire(uint8_t data)
{
    uint8_t i=0;
    OUT_DQ();
    for(i=0;i<8;i++)
    {
        CLR_DQ();
        if(data&0x01)
        {
        SET_DQ();
        }
        else
        {
        CLR_DQ();
        }
        data=data>>1;
        HalWaitUs(6);  //65
        SET_DQ();
    }
}

//-----DS18B20转换温度-----
static void convertDs18b20(void) 
{ 
    resetOnewire(); 
    wOnewire(0xcc); 
    wOnewire(0x44); 
}

//------------DS18BB0读温度----------
uint8_t DS18B20ReadTemp(void) 
{ 
    uint8_t temp1,temp2;
    convertDs18b20();
    resetOnewire(); 
    wOnewire(0xcc); 
    wOnewire(0xbe);         
    temp1=rOnewire(); 
    temp2=rOnewire(); 
    temp2=temp2<<4;
    temp1=temp1>>4;
    temp2|=temp1;
    return (temp2&0x7F);
}

