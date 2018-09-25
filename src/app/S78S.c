#include "S78S.h"
#include "VTStaticQueue.h"

typedef enum
{
    S78S_MODE_IDLE = 0,
    S78S_MODE_RESET,
    S78S_MODE_ONLINE,
    S78S_MODE_SLEEP,
}S78SMode_t;

typedef enum
{
    S78S_CMD_NONE = 0,
    S78S_CMD_SET_DEVEUI,
    S78S_CMD_SET_APPEUI,
    S78S_CMD_SET_APPKEY,
    S78S_CMD_JOIN_OTAA,
    S78S_CMD_TX_CNF,
    S78S_CMD_SIP_RESET,
    S78S_CMD_SIP_SLEEP,
}S78SCmd_t;

static S78SCmd_t g_currentCmd = S78S_CMD_NONE;

static volatile uint8_t g_recvFlag = FALSE;
static S78SMode_t g_mode = S78S_MODE_IDLE;

static uint8_t g_buff[128];
static uint8_t g_buffCount = 0;

static VTSQueueDef(uint8_t, g_dataQueue, 256);
static uint32_t g_cmdLastSendTime = 0;
static uint32_t g_cmdTimeoutCount = 0;

static void cmdSend(S78SCmd_t cmd, const char *text)
{
    HalUartWrite((uint8_t *)text, strlen(text));
    g_currentCmd = cmd;
    g_cmdLastSendTime = HalTime();
    if(cmd == S78S_CMD_JOIN_OTAA || cmd == S78S_CMD_TX_CNF)
    {
        g_cmdTimeoutCount = 30000;
    }
    else
    {
        g_cmdTimeoutCount = 5000;
    }
}

void S78SDataRecv(uint8_t byte)
{
    static uint8_t lastByte = 0;
    
    if(g_recvFlag)
    {
        if(VTSQueueHasSpace(g_dataQueue))
        {
            VTSQueuePush(g_dataQueue, byte);
        }
        
        if(byte == '\n')
        {
            g_recvFlag = FALSE; //end
        }
    }
    else
    {
        if(lastByte == '>' && byte == '>') //head flag
        {
            g_recvFlag = TRUE;
        }
        lastByte = byte;
    }
}

extern void APPPacketRecv(char *pack);
static bool keywordHandle(char *frame)
{
    char *temp;
    if(strstr(frame, "mac rx"))
    {
        temp = strstr(frame, "mac rx");
        temp = strchr(temp, ' ');
        temp = strchr(temp + 1, ' ');
        temp = strchr(temp + 1, ' ');
        APPPacketRecv(temp);
        return TRUE;
    }
    
    return FALSE;
}

static void frameHandle(char *frame)
{
    char text[64] = "";
    if(keywordHandle(frame))
    {
        return;
    }
    switch(g_currentCmd)
    {
        case S78S_CMD_NONE:
            if(strstr(frame, "S78S"))
            {
                g_mode = S78S_MODE_RESET;
                sprintf(text, "mac set_deveui %s", HalGetDeveui());
                cmdSend(S78S_CMD_SET_DEVEUI, text);
            }
            break;
        case S78S_CMD_SET_DEVEUI:
            if(strstr(frame, "Ok"))
            {
                sprintf(text, "mac set_appeui %s", HalGetAppeui());
                cmdSend(S78S_CMD_SET_APPEUI, text);
            }
            break;
        case S78S_CMD_SET_APPEUI:
            if(strstr(frame, "Ok"))
            {
                sprintf(text, "mac set_appkey %s", HalGetAppkey());
                cmdSend(S78S_CMD_SET_APPKEY, text);
            }
            break;
        case S78S_CMD_SET_APPKEY:
            if(strstr(frame, "Ok"))
            {
                cmdSend(S78S_CMD_JOIN_OTAA, "mac join otaa");
            }
            break;
        case S78S_CMD_JOIN_OTAA:
            if(strstr(frame, "accepted"))
            {
                g_mode = S78S_MODE_ONLINE;
                g_currentCmd = S78S_CMD_NONE;
                //cmdSend(S78S_CMD_TX_CNF, "mac tx cnf 8 0102030405060708ff");
            }
            break;
        case S78S_CMD_TX_CNF:
            if(strstr(frame, "tx_ok"))
            {
                g_currentCmd = S78S_CMD_NONE;
            }
            break;
        case S78S_CMD_SIP_RESET:
            if(strstr(frame, "S78S"))
            {
                g_mode = S78S_MODE_RESET;
                sprintf(text, "mac set_deveui %s", HalGetDeveui());
                cmdSend(S78S_CMD_SET_DEVEUI, text);
            }
            break;
        case S78S_CMD_SIP_SLEEP:
            break;
        default:
            break;
    }
}

bool S78SIsOnline(void)
{
    return (g_mode == S78S_MODE_ONLINE);
}

static void s78sFrameGot(void)
{
    uint8_t byte;
    while(VTSQueueCount(g_dataQueue))
    {
        HalInterruptSet(FALSE);
        byte = VTSQueueFront(g_dataQueue);
        VTSQueuePop(g_dataQueue);
        HalInterruptSet(TRUE);
        g_buff[g_buffCount++] = byte;
        if(byte == '\n')
        {
            g_buff[g_buffCount] = '\0';
            frameHandle((char *)g_buff);
            g_buffCount = 0;
            break;
        }
    }
}

static void cmdSendTimeoutHandle(void)
{    
    if(g_currentCmd != S78S_CMD_NONE)
    {
        if(HalTime() - g_cmdLastSendTime > g_cmdTimeoutCount)

        {
            if(g_currentCmd == S78S_CMD_SIP_RESET)
            {
                HalReboot();
            }
            g_mode = S78S_MODE_IDLE;
            g_currentCmd = S78S_CMD_NONE;
        }
    }
}

static void s78sDetectPoll(void)
{
    if(HalTime() > 1000 && g_mode == S78S_MODE_IDLE && g_currentCmd == S78S_CMD_NONE)
    {
        cmdSend(S78S_CMD_SIP_RESET, "sip reset");
    }
}

void S78SDataSend(char *data)
{
    uint16_t i;
    char *temp;
    char *cmd = "mac tx cnf 8 ";
    uint16_t len = strlen(data) * 2;
    char *buff;

    if(g_currentCmd != S78S_CMD_NONE)
    {
        return;
    }
    buff = (char *)malloc(len + strlen(cmd) + 1);

    if(buff)
    {
        memset(buff, 0, len + strlen(cmd) + 1);
        strcpy(buff, cmd);
        temp = &buff[strlen(cmd)];
        for(i = 0; i < strlen(data); i++)
        {
            sprintf(temp, "%02x", data[i]);
            temp += 2;
        }
        
        cmdSend(S78S_CMD_TX_CNF, buff);
        free(buff);
    }
}

void S78SInitialize(void)
{
}

void S78SPoll(void)
{
    s78sFrameGot();
    s78sDetectPoll();
    cmdSendTimeoutHandle();
}

