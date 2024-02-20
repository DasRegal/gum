#include <stdint.h>
#include <string.h>

#include "stm32f10x.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "timers.h"
#include "semphr.h"

#include "main.h"

#include "coinbox.h"
#include "src/flow/flow.h"

#define CCTALK_CLI_CMD_FLAG     (1)
#define CCTALK_SEQ_1_CMD_FLAG   (1 << 1)
#define CCTALK_SEQ_2_CMD_FLAG   (1 << 2)
#define CCTALK_SEQ_3_CMD_FLAG   (1 << 3)
#define CCTALK_SEQ_4_CMD_FLAG   (1 << 4)

typedef struct
{
    uint8_t hdr;
    uint8_t data[CCTALK_MAX_BUF_LEN];
    uint8_t size;
    uint32_t balance;
    uint8_t  balance_counter;
} coinbox_data_t;

static EventGroupHandle_t  xCCTalkEventGroup;
static SemaphoreHandle_t   xBalanceMutex;
static bool is_balance_change = false;
static QueueHandle_t fdBuferCctalk;
static volatile char*    pCBTxData;
static volatile uint8_t  CBTxLen;
static coinbox_data_t coinbox_data;

static void CctalkUsartInit(void);
static void vTaskCctalkSend ( void *pvParameters);
static void vTaskCctalkReceive(void *pvParameters);
void UsartSendString_Cctalk(const char *pucBuffer);

void CoinBoxInit(void)
{
    cctalk_master_dev_t dev;
    dev.send_data_cb = UsartSendString_Cctalk;
    coinbox_data.balance = 0;
    coinbox_data.balance_counter = 0;

    fdBuferCctalk = xQueueCreate(256, sizeof(uint8_t));
    CctalkInit(dev);
    CctalkUsartInit();

    xCCTalkEventGroup = xEventGroupCreate();

    xBalanceMutex = xSemaphoreCreateMutex();

    xTaskCreate(vTaskCctalkReceive, (const char*)"cctalk rx", 500, NULL, tskIDLE_PRIORITY + 1, (TaskHandle_t*)NULL);
    xTaskCreate(vTaskCctalkSend,    (const char*)"cctalk tx", 256, NULL, tskIDLE_PRIORITY + 1, (TaskHandle_t*)NULL);
}

static void vTaskCctalkReceive(void *pvParameters)
{
    uint8_t              ch;
    cctalk_master_dev_t *dev;

    dev = CctalkGetDev();

    for(;;)
    {
        if (xQueueReceive(fdBuferCctalk, &ch, 1000) == pdPASS)
        {
            xSemaphoreTake(xBalanceMutex, portMAX_DELAY);
                if(CctalkGetCharHandler(ch))
                {
                    if (dev->credit.balance)
                    {
                        is_balance_change = true;
                        FlowBalanceUpdateCb(dev->credit.balance);
                        dev->credit.balance = 0;
                    }
                    CctalkAnswerHandle();
                }
            xSemaphoreGive(xBalanceMutex);
        }
        else
        {
            /* Reset */
            xEventGroupSetBits(xCCTalkEventGroup, CCTALK_SEQ_1_CMD_FLAG);
        }
    }
}

static void vTaskCctalkSend ( void *pvParameters)
{
    EventBits_t flags;

    vTaskDelay(500);
    xEventGroupSetBits(xCCTalkEventGroup, CCTALK_SEQ_1_CMD_FLAG);

    /**
      * data[0] = 7;
      * CoinBoxCliCmdSendData(CCTALK_HDR_REQ_COIN_ID, data, 1);
      * 1 RU100A
      * 2 RU100B
      * 3 RU200A
      * 4 RU200B
      * 5 RU500A
      * 6 RU500B
      * 7 RU1K0A
      * 8 RU1K0B 
      */

    for( ;; )
    {
        flags = xEventGroupGetBits(xCCTalkEventGroup);

        if (flags & CCTALK_CLI_CMD_FLAG)
        {
            xEventGroupClearBits(xCCTalkEventGroup, CCTALK_CLI_CMD_FLAG);
            CctalkSendData(coinbox_data.hdr, coinbox_data.data, coinbox_data.size);
            continue;
        }

        if (flags & CCTALK_SEQ_1_CMD_FLAG)
        {
            uint8_t data[1];
            xEventGroupClearBits(xCCTalkEventGroup, CCTALK_SEQ_1_CMD_FLAG);
            data[0] = 0;
            CctalkSendData(CCTALK_HDR_MOD_MASTER_INH_STAT, data, 0);
            vTaskDelay(200);
            xEventGroupSetBits(xCCTalkEventGroup, CCTALK_SEQ_2_CMD_FLAG);
            continue;
        }

        if (flags & CCTALK_SEQ_2_CMD_FLAG)
        {
            uint8_t data[2];
            xEventGroupClearBits(xCCTalkEventGroup, CCTALK_SEQ_2_CMD_FLAG);
            data[0] = 0;
            data[1] = 0;
            CctalkSendData(CCTALK_HDR_MOD_INH_STAT, data, 2);
            vTaskDelay(200);
            xEventGroupSetBits(xCCTalkEventGroup, CCTALK_SEQ_3_CMD_FLAG);
            continue;
        }

        if (flags & CCTALK_SEQ_3_CMD_FLAG)
        {
            uint8_t data[2];
            xEventGroupClearBits(xCCTalkEventGroup, CCTALK_SEQ_3_CMD_FLAG);
            data[0] = 255;
            data[1] = 255;
            CctalkSendData(CCTALK_HDR_MOD_INH_STAT, data, 2);
            vTaskDelay(200);
            xEventGroupSetBits(xCCTalkEventGroup, CCTALK_SEQ_4_CMD_FLAG);
            continue;
        }

        if (flags & CCTALK_SEQ_4_CMD_FLAG)
        {
            xEventGroupClearBits(xCCTalkEventGroup, CCTALK_SEQ_4_CMD_FLAG);
            CctalkSendData(CCTALK_HDR_REQ_INH_STAT, NULL, 0);
            vTaskDelay(200);
            continue;
        }

        CctalkSendData(CCTALK_HDR_READ_BUF_CREDIT, NULL, 0);
        vTaskDelay(500);
    }
}

void CoinBoxGetData(char **buf, uint8_t *len)
{
    cctalk_master_dev_t *dev;
    dev = CctalkGetDev();
    *buf = dev->buf;
    *len = dev->buf_data_len;
}

bool CoinBoxIsUpdateBalance(uint32_t *balance)
{
    cctalk_master_dev_t *dev;
    uint32_t val;

    dev = CctalkGetDev();

    xSemaphoreTake(xBalanceMutex, portMAX_DELAY);
        if (!is_balance_change)
        {
            return false;
        }

        is_balance_change = false;
        val = dev->credit.balance;
        dev->credit.balance = 0;
        *balance = val;
    xSemaphoreGive(xBalanceMutex);
    return true;
}

void CoinBoxCliCmdSendData(uint8_t hdr, uint8_t *buf, uint8_t len)
{
    coinbox_data.hdr = hdr;
    coinbox_data.size = len;
    if (buf != NULL)
    {
        for (uint8_t i = 0; i < len; i++)
            coinbox_data.data[i] = buf[i];
    }

    xEventGroupSetBits(xCCTalkEventGroup, CCTALK_CLI_CMD_FLAG);
}

static void CctalkUsartInit(void)
{
    USART_InitTypeDef USART_InitStructure;
    GPIO_InitTypeDef  GPIO_InitStructure;
    NVIC_InitTypeDef  NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,  ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
    GPIO_PinRemapConfig(GPIO_PartialRemap_USART3, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOC, &GPIO_InitStructure );

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = 9600;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No ;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART3, &USART_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = configLIBRARY_LOWEST_INTERRUPT_PRIORITY;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(USART3, ENABLE);
    USART_ITConfig(USART3, USART_IT_RXNE, DISABLE);
    USART_ITConfig(USART3, USART_IT_TXE, DISABLE);
    USART_ITConfig(USART3, USART_IT_TC, DISABLE);
}

void UsartSendString_Cctalk(const char *pucBuffer)
{
    pCBTxData = (char*)pucBuffer;
    CBTxLen = pucBuffer[1] + 5;
    USART_ITConfig(USART3, USART_IT_RXNE, DISABLE);
    USART_ITConfig(USART3, USART_IT_TXE, ENABLE);
}

void UART3InterruptHandler(void)
{
    if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
    {
        portBASE_TYPE true_false;
        uint8_t ch;

        ch = (uint8_t)USART_ReceiveData(USART3);
        USART_ClearITPendingBit(USART3, USART_IT_RXNE);
        xQueueSendFromISR(fdBuferCctalk, &ch, &true_false);
    }
   
    if(USART_GetITStatus(USART3, USART_IT_TXE) != RESET)
    {
        USART_ClearITPendingBit(USART3, USART_IT_TXE);
        USART_SendData(USART3, *pCBTxData++);
        if (!(--CBTxLen))
        {
            USART_ITConfig(USART3, USART_IT_TXE, DISABLE);
            USART_ITConfig(USART3, USART_IT_TC, ENABLE);
        }
    }
    if(USART_GetITStatus(USART3, USART_IT_TC) != RESET)
    {
        USART_ClearITPendingBit(USART3, USART_IT_TC);
        USART_ITConfig(USART3, USART_IT_TC, DISABLE);

        USART_ReceiveData(USART3);
        USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
    }
}
