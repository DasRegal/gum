#include <stdint.h>
#include <string.h>

#include "stm32f10x.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "timers.h"

#include "main.h"

#include "coinbox.h"

#define CCTALK_CLI_CMD_FLAG     (1)

EventGroupHandle_t  xCCTalkEventGroup;

typedef struct
{
    uint8_t hdr;
    uint8_t data[CCTALK_MAX_BUF_LEN];
    uint8_t size;
    uint32_t balance;
    uint8_t  balance_counter;
} coinbox_data_t;

static coinbox_data_t coinbox_data;

static void CctalkUsartInit(void);
// static uint8_t CoinBoxGetHeader(void);
void vTaskCoinBoxPoll ( void *pvParameters);

QueueHandle_t fdBuferCctalk;

void vTaskCctalk(void *pvParameters)
{
    uint8_t ch;
    QueueHandle_t fdBuferCctalk;
    fdBuferCctalk = (QueueHandle_t*)pvParameters;
    for(;;)
    {
        if (xQueueReceive(fdBuferCctalk, &ch, portMAX_DELAY) == pdPASS)
        {
            if(CctalkGetCharHandler(ch))
            {
                // header = CoinBoxGetHeader();
                // switch(header)
                // {
                //     case CCTALK_HDR_READ_BUF_CREDIT:
                //         break;
                //     default:
                //         break;
                // }

                CctalkAnswerHandle();
            }
        }
    }
}

void CoinBoxGetData(char **buf, uint8_t *len)
{
    cctalk_master_dev_t *dev;
    dev = CctalkGetDev();
    *buf = dev->buf;
    *len = dev->buf_data_len;
}

// static uint8_t CoinBoxGetHeader(void)
// {
//     // cctalk_master_dev_t *dev;
//     // dev = CctalkGetDev();
//     // return dev->buf_tx[3];
// }

// static bool CoinBoxIsUpdateCreditData(void)
// {
//     // cctalk_master_dev_t *dev;
//     // dev = CctalkGetDev();
//     // dev->buf;

//     // if((dev->buf[0]) != coinbox_data.balance_counter)
//     // {
//     //     coinbox_data.balance_counter = dev->buf[0];
//     // }
// }

// bool CoinBoxIsUpdateBalance(uint32_t *balance)
// {

// }

void vTaskCoinBoxPoll ( void *pvParameters)
{
    EventBits_t flags;

    uint8_t data[10];
    vTaskDelay(500);
    data[0] = 0;
    CctalkSendData(CCTALK_HDR_MOD_MASTER_INH_STAT, data, 0);
    vTaskDelay(200);
    data[0] = 0;
    data[1] = 0;
    CctalkSendData(CCTALK_HDR_MOD_INH_STAT, data, 2);
    vTaskDelay(200);
    data[0] = 255;
    data[1] = 255;
    CctalkSendData(CCTALK_HDR_MOD_INH_STAT, data, 2);
    vTaskDelay(200);
    CctalkSendData(CCTALK_HDR_REQ_INH_STAT, NULL, 0);
    vTaskDelay(200);

    data[0] = 1;
    CctalkSendData(184, data, 1);
    vTaskDelay(200);

    data[0] = 2;
    CctalkSendData(184, data, 1);
    vTaskDelay(200);
        data[0] = 3;
    CctalkSendData(184, data, 1);
    vTaskDelay(200);
        data[0] = 4;
    CctalkSendData(184, data, 1);
    vTaskDelay(200);
        data[0] = 5;
    CctalkSendData(184, data, 1);
    vTaskDelay(200);
        data[0] = 6;
    CctalkSendData(184, data, 1);
    vTaskDelay(200);
        data[0] = 7;
    CctalkSendData(184, data, 1);
    vTaskDelay(200);
        data[0] = 8;
    CctalkSendData(184, data, 1);
    vTaskDelay(200);

    for( ;; )
    {
        flags = xEventGroupGetBits(xCCTalkEventGroup);

        if (flags & CCTALK_CLI_CMD_FLAG)
        {
            xEventGroupClearBits(xCCTalkEventGroup, CCTALK_CLI_CMD_FLAG);
            CctalkSendData(coinbox_data.hdr, coinbox_data.data, coinbox_data.size);
            continue;
        }

        CctalkSendData(CCTALK_HDR_READ_BUF_CREDIT, NULL, 0);
        vTaskDelay(500);
    }
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

void CoinBoxInit(void)
{
    cctalk_master_dev_t dev;
    dev.send_data_cb = UsartSendString_Cctalk;
    coinbox_data.balance = 0;
    coinbox_data.balance_counter = 0;

    fdBuferCctalk = xQueueCreate(256, sizeof(uint8_t));
    // cctalk_dev_t cctalk_dev;
    CctalkInit(dev);
    CctalkUsartInit();

    xCCTalkEventGroup = xEventGroupCreate();
    // vSemaphoreCreateBinary(cctalk_transfer_sem);

    xTaskCreate(vTaskCctalk, (const char*)"cctalk", 256, (void*)fdBuferCctalk, tskIDLE_PRIORITY + 1, (TaskHandle_t*)NULL);
    xTaskCreate(vTaskCoinBoxPoll, (const char*)"CctalkPoll", 256, NULL, tskIDLE_PRIORITY + 1, (TaskHandle_t*)NULL);
}

void CctalkUsartInit(void)
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


    //  char buf[10];
    // cctalk_data_t data = { .buf = buf };
    // data.dest_addr = 2;
    // data.buf_len = 0;
    // data.src_addr = 1;
    // data.header = 242;
    // CctalkSendData(data);

    // for (int i = 0; i < 5000; i++);
    // char buf[20];
    // // strcpy(buf, "Hello");
    // // UsartSendString_Cctalk(buf);

    // // Req equipment category id "Coin Acceptor"
    // char buf[10] = {2, 0, 1, 246, 7, '\n'};
    // buf[0] = 2;
    // buf[1] = 0;
    // buf[2] = 1;
    // buf[3] = 245;
    // buf[4] = 8;
    // for (char i = 0; i < 5; i++)
    // {
    // while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
    // USART_SendData(USART3, buf[i] );

    // }

    // Req product code "SCA1"
// for (int i = 0; i < 50000; i++);
//      buf[0] = 2;
//     buf[1] = 0;
//     buf[2] = 1;
//     buf[3] = 244;
//     buf[4] = 9;
//     for (char i = 0; i < 5; i++)
//     {
//     while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
//     USART_SendData(USART3, buf[i] );

//     }

//     // Req build code "Standard"
// // for (int i = 0; i < 50000; i++);
    //  buf[0] = 2;
    // buf[1] = 0;
    // buf[2] = 1;
    // buf[3] = 192;
    // buf[4] = 61;
//     for (char i = 0; i < 5; i++)
//     {
//     while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
//     USART_SendData(USART3, buf[i] & 0x00FF );

//     }

    // Serial number "C16B3D"
    // Received data : [ serial 1 ] [ serial 2 ] [ serial 3 ]
    // = [ serial 1 ] + 256 * [ serial 2 ] + 65536 * [ serial 3 ]
    // 3997696 + 27392 + 193 = 4025281
// for (int i = 0; i < 50000; i++);
    //  buf[0] = 2;
    // buf[1] = 0;
    // buf[2] = 1;
    // buf[3] = 242;
    // buf[4] = 11;
    // for (char i = 0; i < 5; i++)
    // {
    // while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
    // USART_SendData(USART3, buf[i] & 0x00FF );

    // }

//     // Request software revision "SB10C"
// // for (int i = 0; i < 50000; i++);
//      buf[0] = 2;
//     buf[1] = 0;
//     buf[2] = 1;
//     buf[3] = 241;
//     buf[4] = 12;
//     for (char i = 0; i < 5; i++)
//     {
//     while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
//     USART_SendData(USART3, buf[i] );

//     }

//     // Request comms revision "0x01 0x04 0x02" - 1.4.2
// // for (int i = 0; i < 50000; i++);
//     buf[0] = 2;
//     buf[1] = 0;
//     buf[2] = 1;
//     buf[3] = 4;
//     buf[4] = 249;
//     for (char i = 0; i < 5; i++)
//     {
//         while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
//         USART_SendData(USART3, buf[i] );
//     }

    // // Request coin id for coin 1 = ""
    // buf[0] = 2;
    // buf[1] = 2;
    // buf[2] = 1;
    // buf[3] = 231;
    // buf[4] = 255;
    // buf[5] = 255;
    // buf[6] = 22;
    // for (char i = 0; i < 7; i++)
    // {
    //     while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
    //     USART_SendData(USART3, buf[i] );
    // }

    // for (int i = 0; i < 5000; i++);
    // UsartSendString_Cctalk(buf);

}

volatile char*    pCBTxData;
volatile uint8_t CBTxLen;

void UsartSendString_Cctalk(const char *pucBuffer)
{
    pCBTxData = (char*)pucBuffer;
    CBTxLen = pucBuffer[1] + 5;
    USART_ITConfig(USART3, USART_IT_RXNE, DISABLE);
    USART_ITConfig(USART3, USART_IT_TXE, ENABLE);  //запустили прерывание
}

void UART3InterruptHandler(void)
{

    // static  portBASE_TYPE   xHigherPriorityTaskWoken = pdFALSE;
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
        if (!(--CBTxLen))   //если это был последний байт
        {
            USART_ITConfig(USART3, USART_IT_TXE, DISABLE);
            USART_ITConfig(USART3, USART_IT_TC, ENABLE);
        }
    }
    if(USART_GetITStatus(USART3, USART_IT_TC) != RESET)
    {   //всё передали
        USART_ClearITPendingBit(USART3, USART_IT_TC);
        USART_ITConfig(USART3, USART_IT_TC, DISABLE);

        // xSemaphoreGiveFromISR(DebugUsartTxOk    ,&xHigherPriorityTaskWoken  );
        // if( xHigherPriorityTaskWoken == pdTRUE )    taskYIELD();

        USART_ReceiveData(USART3);
        USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
    }
}