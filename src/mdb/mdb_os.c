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
static void MdbOsUpdateNonRespTime(uint8_t time);
static void MdbOsSetupSeq(EventBits_t flags);
static void MdbOsVendSeq(EventBits_t flags);
static void MdbOsReset(void);

void VmcChooseItem(uint16_t price, uint16_t item);

QueueHandle_t       fdBuferMdbRec;
QueueHandle_t       fdBufMdbData;
SemaphoreHandle_t   mdb_transfer_sem;
// SemaphoreHandle_t   mdb_start_rx_sem;
SemaphoreHandle_t   mdb_poll_sem;
EventGroupHandle_t  xCreatedEventGroup;
EventGroupHandle_t  xSetupSeqEg; 
EventGroupHandle_t  xVendSeqEg;
EventGroupHandle_t  xVendStatus; 
TimerHandle_t       xNonResponseTimer;

static uint8_t mdb_count_non_resp;

void MdbDelay()
{
    for(int i = 0; i < 10000; i++)
    {
        asm("NOP");
    }
}

#define MDB_OS_ACK_FLAG         (1)
#define MDB_OS_SELECT_ITEM      (1 << 1)
#define MDB_OS_VEND_APPROVED    (1 << 2)
#define MDB_OS_SESS_CANCEL      (1 << 3)
#define MDB_OS_VEND_DENIED      (1 << 4)
#define MDB_OS_ENABLE_CASHLESS  (1 << 5)
#define MDB_OS_DISABLE_CASHLESS (1 << 6)
#define MDB_OS_MAKE_PURCHASE    (1 << 7)

#define MDB_OS_IS_SETUP_FLAG    (1)
#define MDB_OS_SETUP_1_FLAG     (1 << 1)
#define MDB_OS_SETUP_2_FLAG     (1 << 2)
#define MDB_OS_SETUP_3_FLAG     (1 << 3)
#define MDB_OS_SETUP_4_FLAG     (1 << 4)
#define MDB_OS_SETUP_5_FLAG     (1 << 5)
#define MDB_OS_SETUP_6_FLAG     (1 << 6)

#define MDB_OS_IS_MAKE_PURCHASE_FLAG    (1)
#define MDB_OS_PURCHASE_VEND_APPROVED   (1 << 1)
#define MDB_OS_PURCHASE_VEND_DENIED     (1 << 2)

#define MDB_OS_VEND_STATUS_APPROVED  (1 << 0)
#define MDB_OS_VEND_STATUS_DENIED    (1 << 1)

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
                case MDB_RET_IN_PROGRESS:
                    continue;
                case MDB_RET_UPDATE_RESPONSE_TIME:
                    uint8_t nonRespTime = MdbGetNonRespTime();
                    MdbOsUpdateNonRespTime(nonRespTime);
                case MDB_RET_OK_DATA:
                    xEventGroupSetBits(xCreatedEventGroup, MDB_OS_ACK_FLAG);
                    xSemaphoreGive(mdb_transfer_sem);
                    continue;
                case MDB_RET_IDLE:
                    xSemaphoreGive(mdb_transfer_sem);
                    continue;
                case MDB_RET_VEND_APPROVED:
                    xEventGroupSetBits(xVendStatus, MDB_OS_VEND_STATUS_APPROVED);
                    xEventGroupSetBits(xCreatedEventGroup, MDB_OS_VEND_APPROVED);
                    xEventGroupSetBits(xCreatedEventGroup, MDB_OS_ACK_FLAG);
                    xSemaphoreGive(mdb_transfer_sem);
                    continue;
                case MDB_RET_VEND_DENIED:
                    xEventGroupSetBits(xVendStatus, MDB_OS_VEND_STATUS_DENIED);
                    xEventGroupSetBits(xCreatedEventGroup, MDB_OS_VEND_DENIED);
                    xSemaphoreGive(mdb_transfer_sem);
                    continue;
                case MDB_RET_SESS_CANCEL:
                    xEventGroupSetBits(xCreatedEventGroup, MDB_OS_SESS_CANCEL);
                    xEventGroupSetBits(xCreatedEventGroup, MDB_OS_ACK_FLAG);
                    xSemaphoreGive(mdb_transfer_sem);
                    continue;
                case MDB_RET_REVALUE_APPROVED:
                    continue;
                case MDB_RET_REVALUE_DENIED:
                    continue;
                case MDB_RET_REPEAT:
                    /* Check sum is not valid*/
                    continue;
                default:
                    break;
            }
        }
    }
}

void vTaskMdbPoll ( void *pvParameters)
{
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
            continue;
        }

        if (flags & MDB_OS_SELECT_ITEM)
        {
            xEventGroupClearBits(xCreatedEventGroup, MDB_OS_SELECT_ITEM);
            VmcChooseItem(1, 23);
            continue;
        }

        if (flags & MDB_OS_VEND_APPROVED)
        {
            xEventGroupClearBits(xCreatedEventGroup, MDB_OS_VEND_APPROVED);
            uint8_t buf[2];
            uint16_t item = 23;

            buf[0] = (item >> 8) & 0xFF;
            buf[1] = item & 0xFF;
            MdbSendCommand(MDB_VEND_CMD_E, MDB_VEND_SUCCESS_SUBCMD, buf);
            continue;
        }

        if (flags & MDB_OS_VEND_DENIED)
        {
            xEventGroupClearBits(xCreatedEventGroup, MDB_OS_VEND_DENIED);
            MdbSendCommand(MDB_VEND_CMD_E, MDB_VEND_SESS_COMPL_SUBCMD, NULL);
            continue;
        }

        if (flags & MDB_OS_SESS_CANCEL)
        {
            // TODO
            // ОТМЕНА, ВОЗВРАТ ДЕНЕГ

            xEventGroupClearBits(xCreatedEventGroup, MDB_OS_SESS_CANCEL);
            MdbSendCommand(MDB_VEND_CMD_E, MDB_VEND_SESS_COMPL_SUBCMD, NULL);
            continue;
        }

        if (flags & MDB_OS_ENABLE_CASHLESS)
        {
            xEventGroupClearBits(xCreatedEventGroup, MDB_OS_ENABLE_CASHLESS);
            continue;
        }

        if (flags & MDB_OS_DISABLE_CASHLESS)
        {
            xEventGroupClearBits(xCreatedEventGroup, MDB_OS_DISABLE_CASHLESS);
            continue;
        }

        if (xSemaphoreTake(mdb_poll_sem, 0) == pdTRUE)
        {
            MdbSendCommand(MDB_POLL_CMD_E, 0, NULL);
            continue;
        }

        /* Setup sequence */
        flags = xEventGroupGetBits(xSetupSeqEg);
        if (flags)
        {
            xEventGroupClearBits(xSetupSeqEg, flags);
            flags = xEventGroupSetBits(xSetupSeqEg, (flags << 1));
            MdbOsSetupSeq(flags);
            xSemaphoreGive(mdb_poll_sem);
            continue;
        }

        /* Purchase sequence */
        flags = xEventGroupGetBits(xVendSeqEg);
        if (flags)
        {
            xEventGroupClearBits(xVendSeqEg, flags);
            flags = xEventGroupSetBits(xVendSeqEg, (flags << 1));
            MdbOsVendSeq(flags);
            xSemaphoreGive(mdb_poll_sem);
            continue;
        }

        vTaskDelay(MDB_POLL_TIME);
        MdbSendCommand(MDB_POLL_CMD_E, 0, NULL);
        continue;
    }
}

void vTimerNonRespCb( TimerHandle_t xTimer )
{
    /* Do not repeat ACK by timeout*/
    if (DMA_GetCurrDataCounter(DMA1_Channel7) == 1)
    {
        xTimerStop(xNonResponseTimer, 0);
        return;
    }

    MdbBufSend(NULL, 0);

    mdb_count_non_resp--;
    if (mdb_count_non_resp == 0)
    {
        // mdb_count_non_resp = MDB_COUNT_NON_RESP;
        // /* RESTART */
        // xEventGroupSetBits(xSetupSeqEg, MDB_OS_IS_SETUP_FLAG);
        // xSemaphoreGive(mdb_transfer_sem);
        MdbOsReset();
    }
}

static void MdbOsReset(void)
{
    mdv_dev_init_struct_t mdb_dev_struct;

    mdb_count_non_resp = MDB_COUNT_NON_RESP;

    mdb_dev_struct.send_callback        = MdbBufSend;
    mdb_dev_struct.select_item_cb       = MdbOsSelectItem;
    mdb_dev_struct.session_cancel_cb    = MdbOsSessionCancel;
    MdbInit(mdb_dev_struct);

    xSemaphoreTake(mdb_poll_sem, 0);
    xSemaphoreGive(mdb_transfer_sem);
    xEventGroupSetBits(xSetupSeqEg, MDB_OS_IS_SETUP_FLAG);
}

void MdbOsInit(void)
{

    vSemaphoreCreateBinary(mdb_transfer_sem);
    // vSemaphoreCreateBinary(mdb_start_rx_sem);
    vSemaphoreCreateBinary(mdb_poll_sem);

    xNonResponseTimer = xTimerCreate ( "NonRespTimer", 
                           MDB_NON_RESP_TIMEOUT * 1000,
                           pdFALSE,
                           ( void * ) 0,
                           vTimerNonRespCb );
    
    fdBuferMdbRec = xQueueCreate(256, sizeof(uint16_t));
    fdBufMdbData  = xQueueCreate(256, sizeof(uint8_t));
    xCreatedEventGroup = xEventGroupCreate();
    xSetupSeqEg = xEventGroupCreate();
    xVendStatus = xEventGroupCreate();
    xVendSeqEg  = xEventGroupCreate();

    MdbOsReset();

    MdbUsartInit();
    
    xTaskCreate(
        vTaskMdbRecBuf,
        "MdbRecBuf",
        300,
        (void*)fdBuferMdbRec,
        tskIDLE_PRIORITY + 2,
        NULL
    );

    xTaskCreate(
        vTaskMdbPoll,
        "MdbPoll",
        300,
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
        MdbSendCommand(MDB_RESET_CMD_E, 0, NULL);
        return;
    }

    if (flags & MDB_OS_SETUP_2_FLAG)
    {
        buf[0] = MdbGetLevel();
        buf[1] = 0;
        buf[2] = 0;
        buf[3] = 0;
        MdbSendCommand(MDB_SETUP_CMD_E, MDB_SETUP_CONF_DATA_SUBCMD, buf);
        return;
    }

    if (flags & MDB_OS_SETUP_3_FLAG)
    {
        buf[0] = 0;
        buf[1] = 100;   /* Max price */
        buf[2] = 0;
        buf[3] = 0;     /* Min price */
        MdbSendCommand(MDB_SETUP_CMD_E, MDB_SETUP_PRICE_SUBCMD, buf);
        return;
    }

    if (flags & MDB_OS_SETUP_4_FLAG)
    {
        memcpy(buf,               MDB_VMC_MANUFACTURE_CODE, 3);
        memcpy(buf + 3,           serial_number,            12);
        memcpy(buf + 3 + 12,      model_number,             12);
        memcpy(buf + 3 + 12 + 12, sw_version_bcd,           2);
        MdbSendCommand(MDB_EXPANSION_CMD_E, MDB_EXP_REQ_ID_SUBCMD, buf);
        return;
    }

    if (flags & MDB_OS_SETUP_5_FLAG)
    {
        MdbSendCommand(MDB_READER_CMD_E, MDB_READER_ENABLE_SUBCMD, NULL);
    }

    // Last flag
    xEventGroupClearBits(xSetupSeqEg, flags);
}

static void MdbOsVendSeq(EventBits_t flags)
{
    uint8_t buf[36];

    if (flags & MDB_OS_IS_MAKE_PURCHASE_FLAG)
    {
        uint8_t buf[4];

        xQueueReceive(fdBufMdbData, buf + 0, portMAX_DELAY);
        xQueueReceive(fdBufMdbData, buf + 1, portMAX_DELAY);
        xQueueReceive(fdBufMdbData, buf + 2, portMAX_DELAY);
        xQueueReceive(fdBufMdbData, buf + 3, portMAX_DELAY);

        MdbSendCommand(MDB_VEND_CMD_E, MDB_VEND_REQ_SUBCMD, buf);
        return;
    }

    if (flags & MDB_OS_VEND_1_FLAG)
    {
        buf[0] = MdbGetLevel();
        buf[1] = 0;
        buf[2] = 0;
        buf[3] = 0;
        MdbSendCommand(MDB_SETUP_CMD_E, MDB_SETUP_CONF_DATA_SUBCMD, buf);
        return;
    }

    if (flags & MDB_OS_VEND_1_FLAG)
    {
        buf[0] = 0;
        buf[1] = 100;   /* Max price */
        buf[2] = 0;
        buf[3] = 0;     /* Min price */
        MdbSendCommand(MDB_SETUP_CMD_E, MDB_SETUP_PRICE_SUBCMD, buf);
        return;
    }

    if (flags & MDB_OS_VEND_1_FLAG)
    {
        memcpy(buf,               MDB_VMC_MANUFACTURE_CODE, 3);
        memcpy(buf + 3,           serial_number,            12);
        memcpy(buf + 3 + 12,      model_number,             12);
        memcpy(buf + 3 + 12 + 12, sw_version_bcd,           2);
        MdbSendCommand(MDB_EXPANSION_CMD_E, MDB_EXP_REQ_ID_SUBCMD, buf);
        return;
    }

    if (flags & MDB_OS_VEND_1_FLAG)
    {
        MdbSendCommand(MDB_READER_CMD_E, MDB_READER_ENABLE_SUBCMD, NULL);
    }

    // Last flag
    xEventGroupClearBits(xSetupSeqEg, flags);
}

static void MdbBufSend(const uint16_t *pucBuffer, uint8_t len)
{
    if (len)
    {
        DMA_Cmd(DMA1_Channel7, DISABLE);
        DMA_SetCurrDataCounter(DMA1_Channel7, len);
        DMA_Cmd(DMA1_Channel7, ENABLE);
    }

    DMA_ITConfig(DMA1_Channel7, DMA_IT_TC, ENABLE);
    USART_DMACmd(USART2, USART_DMAReq_Tx, ENABLE);
}

static void MdbOsSelectItem(void)
{
    xEventGroupSetBits(xCreatedEventGroup, MDB_OS_SELECT_ITEM);
}

static void MdbOsSessionCancel(void)
{
    xEventGroupSetBits(xCreatedEventGroup, MDB_OS_SESS_CANCEL);
}


static void MdbOsUpdateNonRespTime(uint8_t time)
{
    xTimerChangePeriod(xNonResponseTimer, time + 5, 0);
    xTimerStop(xNonResponseTimer, 0);
}

void DMA1_Channel7_IRQHandler(void)
{
    static  portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    
    if (DMA_GetITStatus(DMA1_IT_TC7) != RESET)
    {
        DMA_ClearITPendingBit(DMA1_IT_TC7);
        USART_DMACmd(USART2, USART_DMAReq_Tx, DISABLE);
        DMA_ITConfig(DMA1_Channel7, DMA_IT_TC, DISABLE);

        // xSemaphoreGiveFromISR(mdb_start_rx_sem, &xHigherPriorityTaskWoken);
        // if(xHigherPriorityTaskWoken == pdTRUE) taskYIELD();

        xTimerStartFromISR(xNonResponseTimer, &xHigherPriorityTaskWoken);
        if(xHigherPriorityTaskWoken == pdTRUE) taskYIELD();
    }
}

void USART2_IRQHandler(void)
{
    static  portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
    {
        uint16_t ch;

        USART_ClearITPendingBit(USART2, USART_IT_RXNE);
        ch = (uint16_t)USART_ReceiveData(USART2);
        xQueueSendFromISR(fdBuferMdbRec, &ch, NULL);

        mdb_count_non_resp = MDB_COUNT_NON_RESP;
        xTimerStopFromISR(xNonResponseTimer, &xHigherPriorityTaskWoken);
        if(xHigherPriorityTaskWoken == pdTRUE) taskYIELD();
    }
}

void VmcChooseItem(uint16_t price, uint16_t item)
{
    uint8_t buf[4];

    buf[0] = (price >> 8) & 0xFF;
    buf[1] = price & 0xFF;
    buf[2] = (item >> 8) & 0xFF;
    buf[3] = item & 0xFF;
    MdbSendCommand(MDB_VEND_CMD_E, MDB_VEND_REQ_SUBCMD, buf);
}

void CashlessEnable(void)
{
    mdb_state_t state;
    state = MdbGetMachineState();

    if (state != MDB_STATE_INACTIVE && state != MDB_STATE_DISABLED)
        return;

    xEventGroupSetBits(xCreatedEventGroup, MDB_OS_ENABLE_CASHLESS);
}

void CashlessDisable(void)
{
    mdb_state_t state;
    state = MdbGetMachineState();

    if (state == MDB_STATE_INACTIVE || state == MDB_STATE_INACTIVE)
        return;

    xEventGroupSetBits(xCreatedEventGroup, MDB_OS_DISABLE_CASHLESS);
}

uint8_t CashlessMakePurchase(uint16_t item, uint16_t price)
{
    uint8_t buf[4];
    mdb_state_t state;
    state = MdbGetMachineState();

    if (state != MDB_STATE_SESSION_IDLE)
        return 1;

    buf[0] = (item >> 8) & 0xFF;
    buf[1] = item & 0xFF;
    buf[2] = (price >> 8) & 0xFF;
    buf[3] = price & 0xFF;

    xQueueReset(fdBufMdbData);
    xQueueSend(fdBufMdbData, buf++, NULL);
    xQueueSend(fdBufMdbData, buf++, NULL);
    xQueueSend(fdBufMdbData, buf++, NULL);
    xQueueSend(fdBufMdbData, buf, NULL);

    xEventGroupClearBits(xVendStatus, MDB_OS_VEND_STATUS_APPROVED | MDB_OS_VEND_STATUS_DENIED);

    xEventGroupSetBits(xCreatedEventGroup, MDB_OS_IS_MAKE_PURCHASE_FLAG);
    return 0;
}

/* User pushes coin mech. escrow return */
void CashlessVendCancel(void)
{
    mdb_state_t state;
    state = MdbGetMachineState();

    if (state != MDB_STATE_VEND)
        return;

    /* VEND CANCEL COMMAND */
}


void CashlessVendSuccess(uint16_t item)
{
    uint8_t buf[2];
    mdb_state_t state;
    state = MdbGetMachineState();

    if (state != MDB_STATE_VEND)
        return;

    buf[0] = (item >> 8) & 0xFF;
    buf[1] = item & 0xFF;

    xQueueReset(fdBufMdbData);
    xQueueSend(fdBufMdbData, buf++, NULL);
    xQueueSend(fdBufMdbData, buf++, NULL);

    xEventGroupSetBits(xCreatedEventGroup, MDB_OS_ACK_FLAG);
    xEventGroupSetBits(xCreatedEventGroup, MDB_OS_IS_MAKE_PURCHASE_FLAG);
    /* VEND SUCCESS COMMAND */
}

/* VMC fails to dispense product */
void CashlessVendFailure(void)
{
    mdb_state_t state;
    state = MdbGetMachineState();

    if (state != MDB_STATE_VEND)
        return;

    /* VEND FAILURE COMMAND */
}

uint8_t CashlessPoll(void)
{

}