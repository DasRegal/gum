#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "stm32f10x.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"
#include "event_groups.h"

#include "mdb.h"
#include "serial.h"

extern const uint8_t serial_number[12];
extern const uint8_t model_number[12];
extern const uint8_t sw_version_bcd[2];

extern void UsartDebugSendString(const char *pucBuffer);

static void MdbBufSend(const uint16_t *pucBuffer, uint8_t len);
static void MdbOsSetupSeq(EventBits_t flags;);

QueueHandle_t     fdBuferMdbRec;
SemaphoreHandle_t mdb_transfer_sem;
SemaphoreHandle_t mdb_start_rx_sem;
EventGroupHandle_t xCreatedEventGroup;
EventGroupHandle_t xSetupSeqEg; 

void MdbDelay()
{
    for(int i = 0; i < 10000; i++)
    {
        asm("NOP");
    }
}

#define MDB_OS_ACK_FLAG         0b0001
#define MDB_OS_TX_READY_FLAG    0b0010

#define MDB_OS_IS_SETUP_FLAG    0b0001
#define MDB_OS_SETUP_1_FLAG     0b0010
#define MDB_OS_SETUP_2_FLAG     0b0100
#define MDB_OS_SETUP_3_FLAG     0b1000

void vTaskMdbRecBuf ( void *pvParameters)
{
    // uint8_t len = 0;

    uint16_t ch;
    mdb_ret_resp_t resp;
    QueueHandle_t fdBuferMdbRec;
    fdBuferMdbRec = (QueueHandle_t*)pvParameters;

    for( ;; )
    {
        // xSemaphoreTake(mdb_start_rx_sem, portMAX_DELAY);

        if (xQueueReceive(fdBuferMdbRec, &ch, portMAX_DELAY) == pdPASS)
        {
            resp = MdbReceiveChar(ch);
            switch(resp)
            {
                case MDB_RET_IN_PROG:
                    continue;
                    break;
                case MDB_RET_DATA:
                    xEventGroupSetBits(xCreatedEventGroup, MDB_OS_ACK_FLAG);
                    xSemaphoreGive(mdb_transfer_sem);
                    continue;
                    break;
                case MDB_RET_IDLE:
                    // xEventGroupSetBits(xCreatedEventGroup, MDB_OS_TX_READY_FLAG);
                    xSemaphoreGive(mdb_transfer_sem);
                    continue;
                    break;
                case MDB_RET_REPEAT:
                    break;
                default:
                    break;
            }
        }


        // len = MDB_MAX_BUF_LEN - DMA_GetCurrDataCounter(DMA1_Channel6); 
        // if (MdbGetRxCh(len - 1) & 0x100)
        // {
        //     DMA_Cmd(DMA1_Channel6, DISABLE);
        //     DMA_SetCurrDataCounter(DMA1_Channel6, MDB_MAX_BUF_LEN);
        //     DMA_Cmd(DMA1_Channel6, ENABLE);
        //     // vTaskDelay(1);
        //     MdbParseData(len);
        //     MdbClearRx(len - 1);
        //     xSemaphoreGive(mdb_transfer_sem);
        //     continue;
        // }
        // xSemaphoreGive(mdb_start_rx_sem);
        // vTaskDelay(5);
    }
}

void vTaskMdbPoll ( void *pvParameters)
{
    // uint8_t state = 0;
    EventBits_t flags;
    for( ;; )
    {
        xSemaphoreTake(mdb_transfer_sem, portMAX_DELAY);

        flags = xEventGroupGetBits(xCreatedEventGroup);

        if (flags & MDB_OS_ACK_FLAG)
        {
            xEventGroupClearBits(xCreatedEventGroup, MDB_OS_ACK_FLAG);
            MdbAckCmd();
            vTaskDelay(5);
            xSemaphoreGive(mdb_transfer_sem);
            //xEventGroupSetBits(xCreatedEventGroup, MDB_OS_TX_READY_FLAG);
            continue;
        }

        // if (flags & MDB_OS_TX_READY_FLAG)
        // {
        //     xEventGroupClearBits(xCreatedEventGroup, MDB_OS_TX_READY_FLAG);
        // }

        flags = xEventGroupGetBits(xSetupSeqEg);
        if (flags & MDB_OS_IS_SETUP_FLAG)
        {
            MdbOsSetupSeq(flags);
        }
        else
        {
            vTaskDelay(MDB_POLL_TIME);
            MdbPollCmd();
        }
    }
}

void MdbOsInit(void)
{
    MdbInit(MdbBufSend);

    vSemaphoreCreateBinary(mdb_transfer_sem);
    vSemaphoreCreateBinary(mdb_start_rx_sem);
    // xSemaphoreTake(mdb_start_rx_sem, 0);

    fdBuferMdbRec = xQueueCreate(256, sizeof(uint16_t));
    xCreatedEventGroup = xEventGroupCreate();
    xSetupSeqEg = xEventGroupCreate();
    xEventGroupSetBits(xSetupSeqEg, MDB_OS_IS_SETUP_FLAG | MDB_OS_SETUP_1_FLAG);
    MdbUsartInit();
    
    xTaskCreate(
        vTaskMdbRecBuf,
        "MdbRecBuf",
        256,
        (void*)fdBuferMdbRec,
        tskIDLE_PRIORITY + 2,
        NULL
    );

    xTaskCreate(
        vTaskMdbPoll,
        "MdbPoll",
        256,
        NULL,
        tskIDLE_PRIORITY + 1,
        NULL
    );
}

static void MdbOsSetupSeq(EventBits_t flags)
{
    uint8_t buf[36];

    if (flags & MDB_OS_SETUP_1_FLAG)
    {
        xEventGroupClearBits(xSetupSeqEg, MDB_OS_SETUP_1_FLAG);
        xEventGroupSetBits(xSetupSeqEg, MDB_OS_SETUP_2_FLAG);
        buf[0] = MDB_LEVEL_1;
        buf[1] = 0;
        buf[2] = 0;
        buf[3] = 0;
        MdbSetupCmd(MDB_SETUP_CONF_DATA_SUBCMD, buf);
        return;
    }

    if (flags & MDB_OS_SETUP_2_FLAG)
    {
        xEventGroupClearBits(xSetupSeqEg, MDB_OS_SETUP_2_FLAG);
        xEventGroupSetBits(xSetupSeqEg, MDB_OS_SETUP_3_FLAG);
        buf[0] = 0;
        buf[1] = 100;   /* Max price */
        buf[2] = 0;
        buf[3] = 0;     /* Min price */
        MdbSetupCmd(MDB_SETUP_PRICE_SUBCMD, buf);
        return;
    }

    if (flags & MDB_OS_SETUP_3_FLAG)
    {
        xEventGroupClearBits(xSetupSeqEg, MDB_OS_SETUP_3_FLAG | MDB_OS_IS_SETUP_FLAG);
        memcpy(buf,               MDB_VMC_MANUFACTURE_CODE, 3);
        memcpy(buf + 3,           serial_number,            12);
        memcpy(buf + 3 + 12,      model_number,             12);
        memcpy(buf + 3 + 12 + 12, sw_version_bcd,           2);
        MdbExpansionCmd(MDB_EXP_REQ_ID_SUBCMD, buf);
        return;
    }
}

static void MdbBufSend(const uint16_t *pucBuffer, uint8_t len)
{
    DMA_Cmd(DMA1_Channel7, DISABLE);
    DMA_SetCurrDataCounter(DMA1_Channel7, len);
    DMA_ITConfig(DMA1_Channel7, DMA_IT_TC, ENABLE);
    DMA_Cmd(DMA1_Channel7, ENABLE);
    // vTaskDelay(5);
    USART_DMACmd(USART2, USART_DMAReq_Tx, ENABLE);
}

void DMA1_Channel7_IRQHandler(void)
{
    if (DMA_GetITStatus(DMA1_IT_TC7) != RESET)
    {
        DMA_ClearITPendingBit(DMA1_IT_TC7);
        USART_DMACmd(USART2, USART_DMAReq_Tx, DISABLE);
        DMA_ITConfig(DMA1_Channel7, DMA_IT_TC, DISABLE);

        static  portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(mdb_start_rx_sem, &xHigherPriorityTaskWoken);
        if(xHigherPriorityTaskWoken == pdTRUE) taskYIELD();
    }
}

void USART2_IRQHandler(void)
{
    if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
    {
        uint16_t ch;

        USART_ClearITPendingBit(USART2, USART_IT_RXNE);
        ch = (uint16_t)USART_ReceiveData(USART2);
        xQueueSendFromISR(fdBuferMdbRec, &ch, NULL);
    }
}