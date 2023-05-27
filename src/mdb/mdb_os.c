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
static void MdbOsSelectItem(void);
static void MdbOsSessionCancel(void);
static void MdbOsVendApproved(void);
static void MdbOsVendDenied(void);
static void MdbOsSetupSeq(EventBits_t flags;);

QueueHandle_t     fdBuferMdbRec;
SemaphoreHandle_t mdb_transfer_sem;
SemaphoreHandle_t mdb_start_rx_sem;
SemaphoreHandle_t mdb_poll_sem;
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

#define MDB_OS_IS_SETUP_FLAG    (1)
#define MDB_OS_SETUP_1_FLAG     (1 << 1)
#define MDB_OS_SETUP_2_FLAG     (1 << 2)
#define MDB_OS_SETUP_3_FLAG     (1 << 3)
#define MDB_OS_SETUP_4_FLAG     (1 << 4)
#define MDB_OS_SETUP_5_FLAG     (1 << 5)

void vTaskMdbRecBuf ( void *pvParameters)
{
    uint16_t ch;
    mdb_ret_resp_t resp;
    QueueHandle_t fdBuferMdbRec;
    fdBuferMdbRec = (QueueHandle_t*)pvParameters;

    for( ;; )
    {
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
        if (xSemaphoreTake(mdb_poll_sem, 0) == pdTRUE)
        {
            MdbPollCmd();
            continue;
        }

        flags = xEventGroupGetBits(xSetupSeqEg);
        if (flags)
        {
            xEventGroupClearBits(xSetupSeqEg, flags);
            flags = xEventGroupSetBits(xSetupSeqEg, (flags << 1));
            MdbOsSetupSeq(flags);
            xSemaphoreGive(mdb_poll_sem);
            continue;
        }
        
        vTaskDelay(MDB_POLL_TIME);
        MdbPollCmd();
        continue;
    }
}

void MdbOsInit(void)
{
    mdv_dev_init_struct_t mdb_dev_struct;

    mdb_dev_struct.send_callback        = MdbBufSend;
    mdb_dev_struct.select_item_cb       = MdbOsSelectItem;
    mdb_dev_struct.session_cancel_cb    = MdbOsSessionCancel;
    mdb_dev_struct.vend_approved_cb     = MdbOsVendApproved;
    mdb_dev_struct.vend_denied_cb       = MdbOsVendDenied;
    MdbInit(mdb_dev_struct);

    vSemaphoreCreateBinary(mdb_transfer_sem);
    vSemaphoreCreateBinary(mdb_start_rx_sem);
    vSemaphoreCreateBinary(mdb_poll_sem);
    
    xSemaphoreTake(mdb_poll_sem, 0);

    fdBuferMdbRec = xQueueCreate(256, sizeof(uint16_t));
    xCreatedEventGroup = xEventGroupCreate();
    xSetupSeqEg = xEventGroupCreate();
    xEventGroupSetBits(xSetupSeqEg, MDB_OS_IS_SETUP_FLAG);
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
        // xEventGroupClearBits(xSetupSeqEg, MDB_OS_SETUP_1_FLAG);
        // xEventGroupSetBits(xSetupSeqEg, MDB_OS_SETUP_2_FLAG);
        MdbResetCmd();
        return;
    }

    if (flags & MDB_OS_SETUP_2_FLAG)
    {
        // xEventGroupClearBits(xSetupSeqEg, MDB_OS_SETUP_2_FLAG);
        // xEventGroupSetBits(xSetupSeqEg, MDB_OS_SETUP_3_FLAG);
        buf[0] = MdbGetLevel();
        buf[1] = 0;
        buf[2] = 0;
        buf[3] = 0;
        MdbSetupCmd(MDB_SETUP_CONF_DATA_SUBCMD, buf);
        return;
    }

    if (flags & MDB_OS_SETUP_3_FLAG)
    {
        // xEventGroupClearBits(xSetupSeqEg, MDB_OS_SETUP_3_FLAG);
        // xEventGroupSetBits(xSetupSeqEg, MDB_OS_SETUP_4_FLAG);
        buf[0] = 0;
        buf[1] = 100;   /* Max price */
        buf[2] = 0;
        buf[3] = 0;     /* Min price */
        MdbSetupCmd(MDB_SETUP_PRICE_SUBCMD, buf);
        return;
    }

    if (flags & MDB_OS_SETUP_4_FLAG)
    {
        // xEventGroupClearBits(xSetupSeqEg, MDB_OS_SETUP_4_FLAG);
        // xEventGroupSetBits(xSetupSeqEg, MDB_OS_SETUP_5_FLAG);
        memcpy(buf,               MDB_VMC_MANUFACTURE_CODE, 3);
        memcpy(buf + 3,           serial_number,            12);
        memcpy(buf + 3 + 12,      model_number,             12);
        memcpy(buf + 3 + 12 + 12, sw_version_bcd,           2);
        MdbExpansionCmd(MDB_EXP_REQ_ID_SUBCMD, buf);
        return;
    }

    if (flags & MDB_OS_SETUP_5_FLAG)
    {
        xEventGroupClearBits(xSetupSeqEg, MDB_OS_SETUP_5_FLAG);
        MdbReaderCmd(MDB_READER_ENABLE_SUBCMD, NULL);
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

static void MdbOsSelectItem(void)
{

}

static void MdbOsSessionCancel(void)
{

}

static void MdbOsVendApproved(void)
{
    
}

static void MdbOsVendDenied(void)
{
    
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