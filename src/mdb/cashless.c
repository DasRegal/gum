#include <stdbool.h>
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
#include "cashless.h"

typedef enum
{
    CL_VEND_APPROVED,
    CL_VEND_DENIED,
    CL_VEND_WAIT   
} cashless_vend_stat_t;

typedef struct
{
    bool isForceEnable;
    bool isForceDisable;
    bool isEnable;
    bool isChange;
    bool isVendRequest;
    bool isProductSelectionTimeout;
    bool isCardReadTimeout;
    bool isReset;
    bool isVendSuccess;
    bool isVendFailure;
    bool isVendCancel;
    cashless_vend_stat_t vendStat;
    uint16_t price;
    uint16_t item;
} cashless_dev_t;

typedef enum
{
    CL_ACT_JST_RST_E,   /* 00H JUST RESET */
    CL_ACT_CFG_E,       /* 01H CONFIG */
    CL_ACT_P_ID_E,      /* 09H PERIPH ID */
    CL_ACT_SES_BEG_E,   /* 03H BEGIN SESSION */
    CL_ACT_SES_CNCL_E,  /* 04H CANCEL SESSION */
    CL_ACT_VEND_APRV,   /* 05H VEND APPROVED */
    CL_ACT_VEND_DEN_E,  /* 06H VEND DENIED */
    CL_ACT_END_SES_E,   /* 07H END SESSION */

    CL_ACT_ACK_E,
    CL_ACT_NACK_E,

    CL_ACT_LAST_E
} cashless_act_t;

typedef enum
{
    CL_ST_RESET_E,          /* 10H RESET */
    CL_ST_POLL_E,           /* 12H POLL */
    CL_ST_SETUP_CONF_E,     /* 11 00H SETUP CONF */
    CL_ST_PRICES_E,         /* 11 01H SETUP PRICES */
    CL_ST_EXP_REQUEST_ID_E, /* 17 00H EXP REQUEST ID */
    CL_ST_EXP_OPT_E,        /* 17 04H EXP OPT */
    CL_ST_EXP_PRICE_E,      /* 11 01H SETUP PRICES */
    CL_ST_ENABLE_E,         /* 14 01H ENABLE */
    CL_ST_DISABLE_E,        /* 14 00H DISABLE */
    CL_ST_VEND_REQ,         /* 13 00H VEND REQUEST */
    CL_ST_SES_CMPL,         /* 13 04H SESSION COMPLETE */
    // CL_ST_VND_APPR,
    // CL_ST_VND_DEN,
    CL_ST_VND_SUCC,         /* 13 02H VEND SUCCESS */
    CL_ST_VND_FAIL,         /* 13 03H VEND FAILURE */
    CL_ST_VND_CAN,          /* 13 01H VEND CANCEL */

    CL_ST_LAST_E
} cashless_state_t;

// cashless_state_t cashless_machine_state[CL_ST_LAST_E][CL_ACT_LAST_E] = 
// { 
//     /* CL_ST_RESET_E        CL_ACT_CFG_E        CL_ACT_P_ID_E       CL_ACT_ACK_E            CL_ACT_NACK_E*/
//     { CL_ST_RESET_E,        CL_ST_RESET_E,      CL_ST_RESET_E,      CL_ST_POLL_E,           CL_ST_RESET_E          }, /* CL_ST_RESET_E */
//     { CL_ST_SETUP_CONF_E,   CL_ST_PRICES_E,     CL_ST_POLL_E, /*CL_ST_EXP_OPT_E,*/    CL_ST_POLL_E,           CL_ST_POLL_E           }, /* CL_ST_POLL_E */
//     { CL_ST_RESET_E,        CL_ST_RESET_E,      CL_ST_RESET_E,      CL_ST_POLL_E,           CL_ST_SETUP_CONF_E     }, /* CL_ST_SETUP_CONF_E */
//     { CL_ST_RESET_E,        CL_ST_RESET_E,      CL_ST_RESET_E,      CL_ST_EXP_REQUEST_ID_E, CL_ST_PRICES_E         }, /* CL_ST_PRICES_E */
//     { CL_ST_RESET_E,        CL_ST_RESET_E,      CL_ST_RESET_E,      CL_ST_POLL_E,           CL_ST_EXP_REQUEST_ID_E }, /* CL_ST_EXP_REQUEST_ID_E */
//     { CL_ST_RESET_E,        CL_ST_RESET_E,      CL_ST_RESET_E,      CL_ST_EXP_PRICE_E,      CL_ST_EXP_OPT_E        }, /* CL_ST_EXP_OPT_E */
//     { CL_ST_RESET_E,        CL_ST_RESET_E,      CL_ST_RESET_E,      CL_ST_POLL_E,           CL_ST_EXP_PRICE_E      }, /* CL_ST_EXP_PRICE_E */
// };

cashless_state_t cashless_machine_state[CL_ST_LAST_E][CL_ACT_LAST_E] = 
{ 
    /* 00 JUST RESET        01 CONFIG           09 PERIPH ID        03 BEGIN SESSION        04 CANCEL SESSION       05 VEND APPROVED        06 VEND DENIED         07 END SESSION          ACK                     NACK         */
    /* CL_ST_RESET_E        CL_ACT_CFG_E        CL_ACT_P_ID_E       CL_ACT_SES_BEG_E        CL_ACT_SES_CNCL_E       CL_ACT_VEND_APRV        CL_ACT_VEND_DEN_E                              CL_ACT_ACK_E            CL_ACT_NACK_E*/
    { CL_ST_RESET_E,        CL_ST_RESET_E,      CL_ST_RESET_E,      CL_ST_RESET_E,          CL_ST_RESET_E,          CL_ST_RESET_E,          CL_ST_RESET_E,         CL_ST_RESET_E,          CL_ST_POLL_E,           CL_ST_RESET_E          }, /* CL_ST_RESET_E          10H RESET */
    { CL_ST_SETUP_CONF_E,   CL_ST_PRICES_E,     CL_ST_POLL_E,       CL_ST_POLL_E,           CL_ST_SES_CMPL,         CL_ST_POLL_E,           CL_ST_SES_CMPL,        CL_ST_POLL_E,           CL_ST_POLL_E,           CL_ST_POLL_E           }, /* CL_ST_POLL_E           12H POLL */
    { CL_ST_POLL_E,         CL_ST_POLL_E,       CL_ST_POLL_E,       CL_ST_RESET_E,          CL_ST_RESET_E,          CL_ST_RESET_E,          CL_ST_RESET_E,         CL_ST_RESET_E,          CL_ST_POLL_E,           CL_ST_SETUP_CONF_E     }, /* CL_ST_SETUP_CONF_E     11 00H SETUP CONF */
    { CL_ST_RESET_E,        CL_ST_RESET_E,      CL_ST_RESET_E,      CL_ST_RESET_E,          CL_ST_RESET_E,          CL_ST_RESET_E,          CL_ST_RESET_E,         CL_ST_RESET_E,          CL_ST_EXP_REQUEST_ID_E, CL_ST_PRICES_E         }, /* CL_ST_PRICES_E         11 01H SETUP PRICES */
    { CL_ST_RESET_E,        CL_ST_RESET_E,      CL_ST_EXP_OPT_E,    CL_ST_RESET_E,          CL_ST_RESET_E,          CL_ST_RESET_E,          CL_ST_RESET_E,         CL_ST_RESET_E,          CL_ST_POLL_E,           CL_ST_EXP_REQUEST_ID_E }, /* CL_ST_EXP_REQUEST_ID_E 17 00H EXP REQUEST ID */
    { CL_ST_RESET_E,        CL_ST_RESET_E,      CL_ST_RESET_E,      CL_ST_RESET_E,          CL_ST_RESET_E,          CL_ST_RESET_E,          CL_ST_RESET_E,         CL_ST_RESET_E,          CL_ST_ENABLE_E,         CL_ST_EXP_OPT_E        }, /* CL_ST_EXP_OPT_E        17 04H EXP OPT */
    { CL_ST_RESET_E,        CL_ST_RESET_E,      CL_ST_RESET_E,      CL_ST_RESET_E,          CL_ST_RESET_E,          CL_ST_RESET_E,          CL_ST_RESET_E,         CL_ST_RESET_E,          CL_ST_POLL_E,           CL_ST_EXP_PRICE_E      }, /* CL_ST_EXP_PRICE_E      11 01H SETUP PRICES */
    { CL_ST_RESET_E,        CL_ST_RESET_E,      CL_ST_RESET_E,      CL_ST_RESET_E,          CL_ST_RESET_E,          CL_ST_RESET_E,          CL_ST_RESET_E,         CL_ST_RESET_E,          CL_ST_POLL_E,           CL_ST_ENABLE_E         }, /* CL_ST_ENABLE_E         14 01H ENABLE */
    { CL_ST_RESET_E,        CL_ST_RESET_E,      CL_ST_RESET_E,      CL_ST_RESET_E,          CL_ST_RESET_E,          CL_ST_RESET_E,          CL_ST_RESET_E,         CL_ST_RESET_E,          CL_ST_POLL_E,           CL_ST_DISABLE_E        }, /* CL_ST_DISABLE_E        14 00H DISABLE */
    { CL_ST_RESET_E,        CL_ST_RESET_E,      CL_ST_RESET_E,      CL_ST_RESET_E,          CL_ST_RESET_E,          CL_ST_POLL_E,           CL_ST_POLL_E,          CL_ST_RESET_E,          CL_ST_POLL_E,           CL_ST_VEND_REQ         }, /* CL_ST_VEND_REQ         13 00H VEND REQUEST */
    { CL_ST_RESET_E,        CL_ST_RESET_E,      CL_ST_RESET_E,      CL_ST_RESET_E,          CL_ST_RESET_E,          CL_ST_RESET_E,          CL_ST_RESET_E,         CL_ST_POLL_E,           CL_ST_POLL_E,           CL_ST_SES_CMPL         }, /* CL_ST_SES_CMPL         13 04H SESSION COMPLETE */
    // { CL_ST_POLL_E,         CL_ST_POLL_E,       CL_ST_POLL_E,       CL_ST_POLL_E,           CL_ST_POLL_E,           CL_ST_POLL_E,           CL_ST_POLL_E,          CL_ST_RESET_E,          CL_ST_POLL_E,           CL_ST_VND_APPR         }, /* CL_ST_VND_APPR         SET APPROVED */
    // { CL_ST_POLL_E,         CL_ST_POLL_E,       CL_ST_POLL_E,       CL_ST_POLL_E,           CL_ST_POLL_E,           CL_ST_POLL_E,           CL_ST_POLL_E,          CL_ST_RESET_E,          CL_ST_POLL_E,           CL_ST_VND_DEN          }, /* CL_ST_VND_DEN          SET DENIED */
    { CL_ST_POLL_E,         CL_ST_POLL_E,       CL_ST_POLL_E,       CL_ST_POLL_E,           CL_ST_POLL_E,           CL_ST_POLL_E,           CL_ST_POLL_E,          CL_ST_POLL_E,           CL_ST_POLL_E,           CL_ST_VND_SUCC         }, /* CL_ST_VND_SUCC         13 02H VEND SUCCESS */
    { CL_ST_POLL_E,         CL_ST_POLL_E,       CL_ST_POLL_E,       CL_ST_POLL_E,           CL_ST_POLL_E,           CL_ST_POLL_E,           CL_ST_POLL_E,          CL_ST_POLL_E,           CL_ST_SES_CMPL,         CL_ST_VND_FAIL         }, /* CL_ST_VND_FAIL         13 03H VEND FAILURE */
    { CL_ST_POLL_E,         CL_ST_POLL_E,       CL_ST_POLL_E,       CL_ST_POLL_E,           CL_ST_POLL_E,           CL_ST_POLL_E,           CL_ST_POLL_E,          CL_ST_POLL_E,           CL_ST_SES_CMPL,         CL_ST_VND_CAN          }, /* CL_ST_VND_CAN          13 01H VEND CANCEL */
};

static cashless_state_t curState;
static cashless_act_t   curAct;
static uint8_t mdb_count_non_resp;
static uint8_t mdb_count_t_resp;
static cashless_dev_t cashless_dev;

// q0 RESET: ACK NAK OTHER-q0
//          q1  q0   

// q1 POLL: 00_JUST_RESET 01_POLL_CONFIG 09_PERI_ID OTHER
//          q2            q3              q5        delay q1

// q2 ACK SETUP_CONF: ACK NAK    OTHER-q0
//                    q1  q2,q0  

// q3 ACK SETUP_PRICE: ACK NAK   OTHER-q0
//                     q4  q3,q0

// q4 EXP_REQ_ID: ACK NAK    OTHER-q0
//                q1  q4,q0

// q5 ACK EXP_ENABLE_OPT: ACK NAK   OTHER-q0
//                        q6  q5,q0
// q6 SETUP_PRICE: ACK NAK      OTHER-q0
//                 q1  q6,q0

static QueueHandle_t fdCashlessBufRx;
static SemaphoreHandle_t cl_transfer_sem;
static SemaphoreHandle_t cl_ack_sem;
static SemaphoreHandle_t xCurActMutex;
TimerHandle_t xNonResponseTimer;
TimerHandle_t xTResponseTimer;
// TimerHandle_t xProductSelectionTimer;
// TimerHandle_t xCardReadTimer;

void vTaskCLRx(void *pvParameters);
void vTaskCLTx(void *pvParameters);
void vTimerNonRespCb( TimerHandle_t xTimer );
void vTimerTRespCb( TimerHandle_t xTimer );
// void vTimerProductSelectionTimeoutCb( TimerHandle_t xTimer );
// void vTimerCardReadTimeoutCb( TimerHandle_t xTimer );

static void CashlessStateReset(void);
static void CashlessStatePoll(void);
static void CashlessStateSetupConf(void);
static void CashlessStateSetupPrices(void);
static void CashlessStateExpRequestId(void);
static void CashlessStateExpOpt(void);
static void CashlessStateExpPrices(void);
static void CashlessStateEnable(void);
static void CashlessStateDisable(void);
static bool CashlessIsEnable(void);
static bool CashlessIsChangeEnDis(void);
static bool CashlessIsVendRequest(void);
static void CashlessStateVendApproved(void);
static void CashlessStateVendDenied(void);
static void CashlessStateVendRequest(void);
static bool CashlessIsProductSelectionTimeout(void);
static bool CashlessIsCardReadTimeout(void);
static void CashlessStateSessionComplete(void);
static bool CashlessIsReset(void);
static void CashlessReset(void);
static bool CashlessIsVendSuccess(void);
static bool CashlessIsVendFailure(void);
static bool CashlessIsVendCancel(void);
static void CashlessStateVendSuccess(void);
static void CashlessStateVendFailure(void);
static void CashlessStateVendCancel(void);

static void MdbOsUpdateNonRespTime(uint8_t time);

static void MdbBufSend(const uint16_t *pucBuffer, uint8_t len);

void CashlessInit(void)
{
    vSemaphoreCreateBinary(cl_ack_sem);
    xSemaphoreTake( cl_ack_sem, ( TickType_t ) 0 );
    vSemaphoreCreateBinary(cl_transfer_sem);
    xCurActMutex = xSemaphoreCreateMutex();
    fdCashlessBufRx = xQueueCreate(256, sizeof(uint16_t));
    xNonResponseTimer = xTimerCreate ( "NonRespTimer", MDB_NON_RESP_TIMEOUT * 1000, pdFALSE, ( void * ) 0, vTimerNonRespCb );
    xTResponseTimer = xTimerCreate ( "TRespTimer", MDB_T_RESPONSE_TIMEOUT + 3, pdFALSE, ( void * ) 0, vTimerTRespCb );
    // xProductSelectionTimer = xTimerCreate ( "SelTimeout", MDB_PRODUCT_SELECTION_TIMEOUT, pdFALSE, ( void * ) 0, vTimerProductSelectionTimeoutCb );
    // xCardReadTimer = xTimerCreate ( "CardTimeout", MDB_CARD_READ_TIMEOUT, pdFALSE, ( void * ) 0, vTimerCardReadTimeoutCb );

    mdv_dev_init_struct_t mdb_dev_struct;
    mdb_dev_struct.send_callback = MdbBufSend;
    MdbInit(mdb_dev_struct);

    CashlessReset();

    MdbUsartInit();

    xTaskCreate(vTaskCLRx, "Cashless_RX", 128, NULL, tskIDLE_PRIORITY + 2, NULL);
    xTaskCreate(vTaskCLTx, "Cashless_TX", 256, NULL, tskIDLE_PRIORITY + 2, NULL);
}

static void CashlessReset(void)
{

    cashless_dev.isForceEnable              = false;
    cashless_dev.isForceDisable             = false;
    cashless_dev.isEnable                   = true;
    cashless_dev.isChange                   = false;
    cashless_dev.isVendRequest              = false;
    cashless_dev.isProductSelectionTimeout  = false;
    cashless_dev.isCardReadTimeout          = false;
    cashless_dev.isReset                    = false;
    cashless_dev.isVendSuccess              = false;
    cashless_dev.isVendFailure              = false;
    cashless_dev.isVendCancel               = false;
    cashless_dev.vendStat                   = CL_VEND_WAIT;

    mdb_count_non_resp = MDB_COUNT_NON_RESP;
    mdb_count_t_resp = 1;
    curAct = CL_ACT_JST_RST_E;
    curState = CL_ST_RESET_E;


    xTimerStop(xNonResponseTimer, 0);
    xTimerStop(xTResponseTimer, 0);

    xSemaphoreTake(cl_ack_sem, ( TickType_t ) 0);
    xSemaphoreGive(cl_transfer_sem);
}

void vTaskCLRx(void *pvParameters)
{
    uint16_t ch;
    mdb_ret_resp_t resp;
    cashless_act_t action;

    for( ;; )
    {
        if (xQueueReceive(fdCashlessBufRx, &ch, portMAX_DELAY) == pdPASS)
        {
            action = curAct;
            resp = MdbReceiveChar(ch);
            switch(resp)
            {
                case MDB_RET_IN_PROGRESS:
                    continue;
                case MDB_RET_ACK:
                    action = CL_ACT_ACK_E;
                    break;
                case MDB_RET_NACK:
                    action = CL_ACT_NACK_E;
                    break;
                case MDB_RET_JUST_RESET:
                    action = CL_ACT_JST_RST_E;
                    xSemaphoreGive(cl_ack_sem);
                    break;
                case MDB_RET_CONFIG:
                    action = CL_ACT_CFG_E;
                    xSemaphoreGive(cl_ack_sem);
                    break;
                case MDB_RET_PERIPH_ID:
                    action = CL_ACT_P_ID_E;
                    xSemaphoreGive(cl_ack_sem);
                    break;
                case MDB_RET_BEGIN_SESSION:
                    // xTimerStart(xProductSelectionTimer, 0);
                    xSemaphoreGive(cl_ack_sem);
                    break;
                case MDB_RET_SESS_CANCEL:
                    action = CL_ACT_SES_CNCL_E;
                    xSemaphoreGive(cl_ack_sem);
                    break;
                case MDB_RET_VEND_APPROVED:
                    // xTimerStop(xCardReadTimer, 0);
                    CashlessStateVendApproved();
                    xSemaphoreGive(cl_ack_sem);
                    break;
                case MDB_RET_VEND_DENIED:
                    // xTimerStop(xCardReadTimer, 0);
                    action = CL_ST_SES_CMPL;
                    CashlessStateVendDenied();
                    xSemaphoreGive(cl_ack_sem);
                    break;
                case MDB_RET_END_SESSION:
                    xSemaphoreGive(cl_ack_sem);
                    break;
                default:
                    continue;
            }
            if(xSemaphoreTake( xCurActMutex, portMAX_DELAY ) == pdTRUE)
            {
                curAct = action;
                xSemaphoreGive( xCurActMutex );
            }

            xSemaphoreGive(cl_transfer_sem);

        }

    }
}
uint8_t poll_count = 1;
void vTaskCLTx(void *pvParameters)
{
    cashless_act_t action;
    cashless_state_t lastState;

    for( ;; )
    {

        xSemaphoreTake(cl_transfer_sem, portMAX_DELAY);

        if(xSemaphoreTake( cl_ack_sem, ( TickType_t ) 0 ) == pdTRUE)
        {
            MdbAckCmd();
            vTaskDelay(2);
            xSemaphoreGive(cl_transfer_sem);
            continue;
        }

        if(xSemaphoreTake( xCurActMutex, portMAX_DELAY ) == pdTRUE)
        {
            action = curAct;
            xSemaphoreGive( xCurActMutex );
        }

        lastState = curState;
        curState = cashless_machine_state[curState][action];

        // if (lastState == CL_ST_POLL_E && curState == CL_ST_POLL_E)
        // {
        //     if (poll_count < 100 && poll_count > 0)
        //     {
        //         poll_count++;
        //     }
        //     else
        //     {
        //         if (poll_count != 0 )
        //         {
        //         poll_count = 0;
        //         curState = CL_ST_DISABLE_E;

        //         }
        //     }
        // }

        if (CashlessIsChangeEnDis())
        {
            if (CashlessIsEnable())
                curState = CL_ST_ENABLE_E;
            else
                curState = CL_ST_DISABLE_E;
        }

        if (CashlessIsVendRequest())
        {
            curState = CL_ST_VEND_REQ;
        }

        if (CashlessIsProductSelectionTimeout())
        {
            curState = CL_ST_SES_CMPL;
        }

        if (CashlessIsCardReadTimeout())
        {
            curState = CL_ST_VND_CAN;
        }

        if(CashlessIsReset())
        {
            curState = CL_ST_RESET_E;
        }

        if(CashlessIsVendSuccess())
        {
            curState = CL_ST_VND_SUCC;
        }

        if(CashlessIsVendFailure())
        {
            curState = CL_ST_VND_FAIL;
        }

        if (CashlessIsVendCancel())
        {
            curState = CL_ST_VND_CAN;
        }

        switch(curState)
        {
            case CL_ST_RESET_E:
                CashlessStateReset();
                break;
            case CL_ST_POLL_E:
                if (lastState == CL_ST_POLL_E)
                {
                    vTaskDelay(MDB_POLL_TIME);
                }
                CashlessStatePoll();
                break;
            case CL_ST_SETUP_CONF_E:
                CashlessStateSetupConf();
                break;
            case CL_ST_PRICES_E:
                CashlessStateSetupPrices();
                break;
            case CL_ST_EXP_REQUEST_ID_E:
                CashlessStateExpRequestId();
                break;
            case CL_ST_EXP_OPT_E:
                CashlessStateExpOpt();
                break;
            case CL_ST_EXP_PRICE_E:
                CashlessStateExpPrices();
                break;
            case CL_ST_ENABLE_E:
                CashlessStateEnable();
                break;
            case CL_ST_DISABLE_E:
                CashlessStateDisable();
                break;
            case CL_ST_VEND_REQ:
                // xTimerStart(xCardReadTimer, 0);
                CashlessStateVendRequest();
                break;
            case CL_ST_SES_CMPL:
                CashlessStateSessionComplete();
                break;
            // case CL_ST_VND_APPR:
            //     CashlessStateVendApproved();
            //     break;
            // case CL_ST_VND_DEN:
            //     CashlessStateVendDenied();
            //     break;
            case CL_ST_VND_SUCC:
                CashlessStateVendSuccess();
                break;
            case CL_ST_VND_FAIL:
                CashlessStateVendFailure();
                break;
            case CL_ST_VND_CAN:
                CashlessStateVendCancel();
                break;
            default:
                break;
        }
    }
}

static void CashlessStateReset(void)
{
    MdbSendCommand(MDB_RESET_CMD_E, 0, NULL);
}

static void CashlessStatePoll(void)
{
    MdbSendCommand(MDB_POLL_CMD_E, 0, NULL);
}

static void CashlessStateSetupConf(void)
{
    uint8_t buf[4] = {0, 0, 0, 0};
    buf[0] = MdbGetLevel();
    MdbSendCommand(MDB_SETUP_CMD_E, MDB_SETUP_CONF_DATA_SUBCMD, buf);
}

static void CashlessStateSetupPrices(void)
{
    uint8_t buf[4] = {0, 0, 0, 0};
    uint8_t nonRespTime = MdbGetNonRespTime();
    
    MdbOsUpdateNonRespTime(nonRespTime);
    
    buf[1] = 100;   /* Max price */
    MdbSendCommand(MDB_SETUP_CMD_E, MDB_SETUP_PRICE_SUBCMD, buf);
}

static void CashlessStateExpRequestId(void)
{
    uint8_t buf[36];

    memcpy(buf,               MDB_VMC_MANUFACTURE_CODE, 3);
    memcpy(buf + 3,           serial_number,            12);
    memcpy(buf + 3 + 12,      model_number,             12);
    memcpy(buf + 3 + 12 + 12, sw_version_bcd,           2);
    MdbSendCommand(MDB_EXPANSION_CMD_E, MDB_EXP_REQ_ID_SUBCMD, buf);
    // MdbSendCommand(MDB_POLL_CMD_E, 0, NULL);
}

static void CashlessStateExpOpt(void)
{
    /* Always Idle */
    uint8_t buf[4] = {0, 0, 0, 0};
    buf[3] = 0b00100000;
    MdbSendCommand(MDB_EXPANSION_CMD_E, MDB_EXP_OPT_FTR_EN_SUBCMD, buf);
}

static void CashlessStateExpPrices(void)
{

}

static void CashlessStateEnable(void)
{
    MdbSendCommand(MDB_READER_CMD_E, MDB_READER_ENABLE_SUBCMD, NULL);
}

static void CashlessStateDisable(void)
{
    MdbSendCommand(MDB_READER_CMD_E, MDB_READER_DISABLE_SUBCMD, NULL);
}

static void CashlessStateVendRequest(void)
{
    uint8_t buf[4];
    buf[0] = (cashless_dev.price >> 8) & 0xFF;
    buf[1] = cashless_dev.price & 0xFF;
    buf[2] = (cashless_dev.item >> 8) & 0xFF;
    buf[3] = cashless_dev.item & 0xFF;

    // xTimerStop(xProductSelectionTimer, 0);
    MdbSendCommand(MDB_VEND_CMD_E, MDB_VEND_REQ_SUBCMD, buf);
}

static void CashlessStateSessionComplete(void)
{
    MdbSendCommand(MDB_VEND_CMD_E, MDB_VEND_SESS_COMPL_SUBCMD, NULL);
}

/*===============*/

static void MdbOsUpdateNonRespTime(uint8_t time)
{
    xTimerChangePeriod(xNonResponseTimer, time + 5, 0);
    xTimerStop(xNonResponseTimer, 0);
}

static bool CashlessIsVendRequest(void)
{
    if (cashless_dev.isVendRequest)
    {
        cashless_dev.isVendRequest = false;
        cashless_dev.vendStat = CL_VEND_WAIT;
        return true;
    }

    return false;
}

static bool CashlessIsProductSelectionTimeout(void)
{
    if (cashless_dev.isProductSelectionTimeout)
    {
        cashless_dev.isProductSelectionTimeout = false;
        return true;
    }

    return false;
}

static bool CashlessIsCardReadTimeout(void)
{
    if (cashless_dev.isCardReadTimeout)
    {
        cashless_dev.isCardReadTimeout = false;
        return true;
    }

    return false;
}

static void CashlessStateVendApproved(void)
{
    // uint8_t buf[2];
    cashless_dev.vendStat = CL_VEND_APPROVED;

    // buf[0] = (cashless_dev.item >> 8) & 0xFF;
    // buf[1] = cashless_dev.item & 0xFF;

    // MdbSendCommand(MDB_VEND_CMD_E, MDB_VEND_SUCCESS_SUBCMD, buf);
}

static void CashlessStateVendSuccess(void)
{
    uint8_t buf[2];

    buf[0] = (cashless_dev.item >> 8) & 0xFF;
    buf[1] = cashless_dev.item & 0xFF;

    MdbSendCommand(MDB_VEND_CMD_E, MDB_VEND_SUCCESS_SUBCMD, buf);
}

static void CashlessStateVendFailure(void)
{
    MdbSendCommand(MDB_VEND_CMD_E, MDB_VEND_FAILURE_SUBCMD, NULL);
}

static void CashlessStateVendCancel(void)
{
    MdbSendCommand(MDB_VEND_CMD_E, MDB_VEND_CANCEL_SUBCMD, NULL);
}

static void CashlessStateVendDenied(void)
{
    cashless_dev.vendStat = CL_VEND_DENIED;
}

uint8_t CashlessVendSuccessCmd(uint16_t item)
{
    // if (cashless_dev.vendStat == CL_VEND_APPROVED)
    // {
        cashless_dev.isVendSuccess = true;
        return 0;
    // }

    // return 1;
}

uint8_t CashlessVendFailureCmd(void)
{
    // if (cashless_dev.vendStat == CL_VEND_APPROVED)
    // {
        cashless_dev.isVendFailure = true;
        return 0;
    // }

    return 1;
}

uint8_t CashlessVendCancelCmd(void)
{
    if (MdbGetMachineState() != MDB_STATE_VEND)
        return 1;

    cashless_dev.isVendCancel = true;
    return 0;
}

static bool CashlessIsVendSuccess(void)
{
    if (cashless_dev.isVendSuccess)
    {
        cashless_dev.isVendSuccess = false;
        return true;
    }

    return false;
}

static bool CashlessIsVendFailure(void)
{
    if (cashless_dev.isVendFailure)
    {
        cashless_dev.isVendFailure = false;
        return true;
    }

    return false;
}

static bool CashlessIsVendCancel(void)
{
    if (cashless_dev.isVendCancel)
    {
        cashless_dev.isVendCancel = false;
        return true;
    }

    return false;
}

void CashlessShowState(uint8_t *state1, uint8_t *state2, uint8_t *state3)
{
    *state1 = (uint8_t)curState;
    *state2 = (uint8_t)MdbGetMachineState();
    *state3 = (uint8_t)curAct;
}

void CashlessEnable(void)
{
    if (MdbGetMachineState() != MDB_STATE_DISABLED)
        return;

    if (!cashless_dev.isForceDisable)
    {
        if (!cashless_dev.isEnable)
        {
            cashless_dev.isEnable = true;
            cashless_dev.isChange = true;
        }
    }
}

void CashlessDisable(void)
{
    if (MdbGetMachineState() != MDB_STATE_ENABLED)
        return;

    if (!cashless_dev.isForceEnable)
    {
        if (cashless_dev.isEnable)
        {
            cashless_dev.isEnable = false;
            cashless_dev.isChange = true;
        }
    }
}

void CashlessEnableForceCmd(bool enable)
{
    if (enable)
    {
        if (!cashless_dev.isForceEnable)
        {
            cashless_dev.isForceEnable  = true;
            cashless_dev.isForceDisable = false;
            cashless_dev.isChange = true;
        }
    }
    else
    {
        if (cashless_dev.isForceEnable)
        {
            cashless_dev.isForceEnable  = false;
            cashless_dev.isChange = true;
        }
    }
}

void CashlessDisableForceCmd(bool disable)
{
    if (disable)
    {
        if (!cashless_dev.isForceDisable)
        {
            cashless_dev.isForceEnable  = false;
            cashless_dev.isForceDisable = true;
            cashless_dev.isChange = true;
        }
    }
    else
    {
        if (cashless_dev.isForceDisable)
        {
            cashless_dev.isForceDisable  = false;
            cashless_dev.isChange = true;
        }
    }
}

static bool CashlessIsChangeEnDis(void)
{
    if (cashless_dev.isChange)
    {
        cashless_dev.isChange = false;
        return true;
    }

    return false;
}

static bool CashlessIsEnable(void)
{
    if (cashless_dev.isForceEnable)
        return true;

    if (cashless_dev.isForceDisable)
        return false;

    return cashless_dev.isEnable;
}

void CashlessResetCmd(void)
{
    cashless_dev.isReset = true;
}

static bool CashlessIsReset(void)
{
    if (cashless_dev.isReset)
    {
        cashless_dev.isReset = false;
        return true;
    }

    return false;
}

uint8_t CashlessVendRequest(uint16_t price, uint16_t item)
{
    if (MdbGetMachineState() != MDB_STATE_SESSION_IDLE)
        return 1;

    cashless_dev.price = price;
    cashless_dev.item  = item;
    cashless_dev.isVendRequest = true;
    return 0;
}

bool CashlessIsVendApproved(uint16_t *item)
{
    if (cashless_dev.vendStat == CL_VEND_APPROVED)
    {
        cashless_dev.vendStat = CL_VEND_WAIT;
        return true;
    }
    return false;
}

/* =======================================*/

void vTimerNonRespCb( TimerHandle_t xTimer )
{
    /* Do not repeat ACK by timeout*/
    if (DMA_GetCurrDataCounter(DMA1_Channel7) == 1)
    {
        xTimerStop(xNonResponseTimer, 0);
        return;
    }

    // MdbBufSend(NULL, 0);

    xTimerStop(xNonResponseTimer, 0);
    xTimerStop(xTResponseTimer, 0);
    CashlessReset();
    // mdb_count_non_resp--;
    // if (mdb_count_non_resp == 0)
    // {
    //     // mdb_count_non_resp = MDB_COUNT_NON_RESP;
    //     // /* RESTART */
    //     // xEventGroupSetBits(xSetupSeqEg, MDB_OS_IS_SETUP_FLAG);
    //     // xSemaphoreGive(mdb_transfer_sem);
    //     CashlessReset();
    // }
}

void vTimerTRespCb( TimerHandle_t xTimer )
{
    /* Do not repeat ACK by timeout*/
    if (DMA_GetCurrDataCounter(DMA1_Channel7) == 1)
    {
        xTimerStop(xTResponseTimer, 0);
        return;
    }

    if (curState == CL_ST_RESET_E)
    {
        if (mdb_count_t_resp != 0)
        {
            mdb_count_t_resp--;
        }
        else
        {
            xTimerStop(xNonResponseTimer, 0);
            vTaskDelay(10 * 1000);
            xTimerStart(xNonResponseTimer, 0);
        }
    }
    MdbBufSend(NULL, 0);
}

// void vTimerProductSelectionTimeoutCb( TimerHandle_t xTimer )
// {
//     xTimerStop(xProductSelectionTimer, 0);
// }

// void vTimerCardReadTimeoutCb( TimerHandle_t xTimer )
// {
//     xTimerStop(xCardReadTimer, 0);
// }

uint8_t isTimerStart = 0;
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
    if( xTimerIsTimerActive( xNonResponseTimer ) != pdFALSE )
    {
        isTimerStart = 1;
    }
    else
    {
        isTimerStart = 0;
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
        xQueueSendFromISR(fdCashlessBufRx, &ch, NULL);

        mdb_count_non_resp = MDB_COUNT_NON_RESP;
        xTimerStopFromISR(xNonResponseTimer, &xHigherPriorityTaskWoken);
        if(xHigherPriorityTaskWoken == pdTRUE) taskYIELD();

        xTimerStopFromISR(xTResponseTimer, &xHigherPriorityTaskWoken);
        if(xHigherPriorityTaskWoken == pdTRUE) taskYIELD();
    }
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
        xTimerStartFromISR(xTResponseTimer, &xHigherPriorityTaskWoken);
        if(xHigherPriorityTaskWoken == pdTRUE) taskYIELD();

        if( !isTimerStart )
        {
            xTimerStartFromISR(xNonResponseTimer, &xHigherPriorityTaskWoken);
            if(xHigherPriorityTaskWoken == pdTRUE) taskYIELD();
        }

    }
}