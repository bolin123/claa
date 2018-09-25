#include "Hal.h"
#include "stm8l15x_usart.h"
#include "stm8l15x_tim4.h"
#include "stm8l15x_tim2.h"

static volatile uint32_t g_timerCount = 0;

static void clkInit(void)
{
    CLK_DeInit();

    CLK_HSICmd(ENABLE);
    CLK_SYSCLKSourceConfig(CLK_SYSCLKSource_HSI);
    CLK_SYSCLKDivConfig(CLK_SYSCLKDiv_1);
    CLK_SYSCLKSourceSwitchCmd(ENABLE);
    while(CLK_GetSYSCLKSource() != CLK_SYSCLKSource_HSI);
    
    CLK_PeripheralClockConfig(CLK_Peripheral_TIM2, ENABLE);
    CLK_PeripheralClockConfig(CLK_Peripheral_TIM4, ENABLE);
    CLK_PeripheralClockConfig(CLK_Peripheral_USART1, ENABLE);
}

static void timer4Init(void)
{
    /* Time base configuration */
    TIM4_TimeBaseInit(TIM4_Prescaler_128, 124); // 1ms
    /* Clear TIM4 update flag */
    TIM4_ClearFlag(TIM4_FLAG_Update);
    /* Enable update interrupt */
    TIM4_ITConfig(TIM4_IT_Update, ENABLE);

    TIM4_Cmd(ENABLE);
}

INTERRUPT_HANDLER(TIM4_UPD_OVF_TRG_IRQHandler, 25)
{
    if(TIM4_GetFlagStatus(TIM4_FLAG_Update) == SET)
    {
        TIM4_ClearFlag(TIM4_FLAG_Update);
        g_timerCount++;
    }
}

static void timer2Init(void)
{
    TIM2_Cmd(DISABLE);
    TIM2_TimeBaseInit(TIM2_Prescaler_2, TIM2_CounterMode_Up, 80); //10us
    TIM2_SetCounter(80);
    TIM2_ClearFlag(TIM2_FLAG_Update);
    TIM2_ARRPreloadConfig(ENABLE);
    TIM2_Cmd(ENABLE);
}

void HalWaitUs(uint16_t tenUs)
{
    uint16_t count = 0;
    
    timer2Init();
    while(count < tenUs)
    {
        if(TIM2_GetFlagStatus(TIM2_FLAG_Update) == SET)
        {
            TIM2_ClearFlag(TIM2_FLAG_Update);
            count++;
        }
    }
    TIM2_Cmd(DISABLE);
}

static void uartInit(void)
{
    
    //GPIO_Init(GPIOC, GPIO_Pin_3, GPIO_Mode_Out_PP_High_Fast);
    //GPIO_Init(GPIOC, GPIO_Pin_2, GPIO_Mode_In_PU_No_IT);
    /* Configure USART Tx as alternate function push-pull  (software pull up)*/
    GPIO_ExternalPullUpConfig(GPIOC, GPIO_Pin_3, ENABLE);
    /* Configure USART Rx as alternate function push-pull  (software pull up)*/
    GPIO_ExternalPullUpConfig(GPIOC, GPIO_Pin_2, ENABLE);
    
    USART_DeInit(USART1);
    USART_Init(USART1, 
                (uint32_t)115200, 
                USART_WordLength_8b, 
                USART_StopBits_1, 
                USART_Parity_No, 
                USART_Mode_Rx | USART_Mode_Tx);
    
    USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    //USART_ITConfig(USART1, USART_IT_TC, ENABLE);
    USART_Cmd(USART1, ENABLE);

}
#if 0
INTERRUPT_HANDLER(USART1_TX_TIM5_UPD_OVF_TRG_BRK_IRQHandler, 27)
{
    if(USART_GetITStatus(USART1, USART_IT_TC) == SET)
    {
        USART_ClearITPendingBit(USART1, USART_IT_TC);
    }
    if(USART_GetITStatus(USART1, USART_IT_OR) == SET)
    {
        USART_ClearITPendingBit(USART1, USART_IT_OR);
    }
}

#endif
//static uint8_t g_buff[64] = {0};
//static uint8_t g_len = 0;
extern void S78SDataRecv(uint8_t byte);

INTERRUPT_HANDLER(USART1_RX_TIM5_CC_IRQHandler, 28)
{
    uint8_t temp;
    if(USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
    {
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
        temp = USART_ReceiveData8(USART1);
        S78SDataRecv(temp);
    }
}

void HalUartWrite(uint8_t *data, uint16_t len)
{
    uint16_t i;

    for(i = 0; i < len; i++)
    {
        USART_SendData8(USART1, data[i]);
        while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
    }
}
#if 0
static void testCmd(void)
{
    static uint32_t lastTime = 0;
    char *temp = "sip get_ver";
    
    if(g_timerCount - lastTime > 1000)
    {
        HalUartWrite((uint8_t *)temp, strlen(temp));
        lastTime = g_timerCount;
    }
}
#endif
const char *HalGetAppkey(void)
{
    return "00112233445566778899aabbccddeeff";
}

const char *HalGetAppeui(void)

{
    return "2c26c5276c000001";
}

const char *HalGetDeveui(void)
{
    return "004a77276c000003";
}

void HalReboot(void)
{
    //WWDG->CR = WWDG_CR_T6;
}

uint32_t HalTime(void)
{
    return g_timerCount;
}

void HalInterruptSet(bool enable)
{
    if(enable)
    {
        enableInterrupts();
    }
    else
    {
        disableInterrupts();
    }
}

void HalInitialize(void)
{
    clkInit();
    enableInterrupts();
    uartInit();
    timer4Init();
}

void HalPoll(void)
{
    //testCmd();
}

