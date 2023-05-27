#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "stm32f10x.h"
#include "FreeRTOS.h"

#include "mdb.h"

#define MDB_CASHLESS_DEV_1_ADDR     0x10
#define MDB_CASHLESS_DEV_2_ADDR     0x60

#define MDB_ACK                     0x00
#define MDB_RET                     0xAA
#define MDB_NAK                     0xFF
#define MDB_CMD_OUT_OF_SEQUENCE     0x0B

#define MDB_MODE_BIT                0x100

#define MDB_RESET_CMD               0
#define MDB_SETUP_CMD               1
#define MDB_POLL_CMD                2
#define MDB_VEND_CMD                3
#define MDB_READER_CMD              4
#define MDB_REVALUE_CMD             5
#define MDB_EXPANSION_CMD           7

#define MDB_POLL_JUST_RESET_RESP     0x00
#define MDB_POLL_CONFIG_RESP         0x01
#define MDB_POLL_DISPLAY_REQ_RESP    0x02
#define MDB_POLL_BEGIN_SESSION_RESP  0x03
#define MDB_POLL_SESS_CANCEL_RESP    0x04
#define MDB_POLL_VEND_APPROVED_RESP  0x05
#define MDB_POLL_VEND_DENIED_RESP    0x06
#define MDB_POLL_END_SESSION_RESP    0x07
#define MDB_POLL_CANCELLED_RESP      0x08
#define MDB_POLL_PERIPH_ID_RESP      0x09
#define MDB_POLL_ERROR_RESP          0x0A
#define MDB_POLL_OUT_OF_SEQ_RESP     0x0B
#define MDB_POLL_REVAL_APPR_RESP     0x0D
#define MDB_POLL_REVAL_DENIED_RESP   0x0E
#define MDB_POLL_REVAL_LIMIT_RESP    0x0F
#define MDB_POLL_TIME_DATA_RESP      0x11
#define MDB_POLL_DAT_ENTRY_REQ_RESP  0x12
#define MDB_POLL_DAT_ENT_CANCEL_RESP 0x13

#define MDB_CURRENCY_CODE_RUB_1     0x643
#define MDB_CURRENCY_CODE_RUB_2     0x810

extern void UsartDebugSendString(const char *pucBuffer);

static void MdbSendCmd(uint8_t cmd, uint8_t subcmd, uint8_t * data, uint8_t len);
// static void MdbSendResponce(uint8_t resp);
static uint16_t MdbCalcChk(uint16_t * buf, uint8_t len);
static bool MdbIsValidateChk(uint16_t * buf, uint8_t len);
static void MdbSendData(uint16_t * buf, uint8_t len);
static mdb_ret_resp_t MdbParseData(uint8_t len);
static void MdbParseResponse(void);
static void MdbSelectItem(void);
static void MdbSessionCancel(void);
static void MdbVendApproved(void);
static void MdbVendDenied(void);
//static void MdbReceiveData(uint16_t * buf, uint8_t len);

typedef enum
{
    MDB_STATE_INACTIVE,
    MDB_STATE_DISABLED,     /* CMD_READER_ENABLE->STATE_ENABLE, CMD_RESET->STATE_INACTIVE*/
    MDB_STATE_ENABLED,      /* CMD_READER_DISABLE->STATE_DISABLE, CMD_RESET->STATE_INACTIVE */
    MDB_STATE_SESSION_IDLE, /* payment event->STATE_SESS_IDLE. CMD_SESSION_COMPLETE->?, CMD_VEND_REQUEST->STATE_VEND, CMD_REVALUE_REQUEST->STATE_REVALUE, CMD_NEG_VEND_REQUEST->STATE_NEG_VEND */
    MDB_STATE_VEND,
    MDB_STATE_REVALUE,      /* Level 2/3 */
    MDB_STATE_NEG_VEND      /* Level 3 */
} mdb_state_t;

//mdb_level_t mdb_level_type;

typedef struct
{
    mdb_level_t     level;
    uint16_t        country_code;
    uint8_t         scale_factor;
    uint8_t         decimal_places;
    uint8_t         max_resp_time;
    uint8_t         misc;
    uint8_t         manufact_code[3];
    uint8_t         serial_num[12];
    uint8_t         model_num[12];
    uint16_t        sw_version;
} mdb_dev_slave_t;

typedef struct
{
    uint8_t         addr;
    mdb_level_t     level;
    bool            is_expansion_en;
    uint16_t        rx_data[MDB_MAX_BUF_LEN];
    uint8_t         rx_len;
    uint16_t        tx_data[MDB_MAX_BUF_LEN];
    uint8_t         send_cmd;
    uint8_t         send_subcmd;
    void            (*send_callback)(const uint16_t*, uint8_t);
    void            (*select_item_cb)(void);
    void            (*session_cancel_cb)(void);
    void            (*vend_approved_cb)(void);
    void            (*vend_denied_cb)(void);
    mdb_dev_slave_t dev_slave;
    mdb_state_t     state;
} mdb_dev_t;

static mdb_dev_t mdb_dev;


void MdbInit(mdv_dev_init_struct_t dev_struct)
{
    mdb_dev.is_expansion_en = false;
    mdb_dev.send_callback   = dev_struct.send_callback;
    mdb_dev.select_item_cb  = dev_struct.select_item_cb;
    mdb_dev.session_cancel_cb = dev_struct.session_cancel_cb;
    mdb_dev.vend_approved_cb= dev_struct.vend_approved_cb;
    mdb_dev.vend_denied_cb  = dev_struct.vend_denied_cb;
    mdb_dev.rx_len          = 0;
    mdb_dev.level           = MDB_LEVEL_2;
    mdb_dev.state           = MDB_STATE_INACTIVE;
    mdb_dev.addr            = MDB_CASHLESS_DEV_1_ADDR;
}

void MdbSetSlaveAddr(uint8_t addr)
{
    mdb_dev.addr = addr;
}

void MdbResetCmd(void)
{
    uint8_t data[1];
    MdbSendCmd(MDB_RESET_CMD, 0, data, 0);
}

void MdbSetupCmd(uint8_t subcmd, uint8_t * data)
{
    uint8_t len = 5;
    if (mdb_dev.is_expansion_en &&
        mdb_dev.level == MDB_LEVEL_3)
    {
        len = 11;
    }

    MdbSendCmd(MDB_SETUP_CMD, subcmd, data, len);
}

void MdbPollCmd(void)
{
    uint8_t data[1];
    MdbSendCmd(MDB_POLL_CMD, 0, data, 0);
}

void MdbVendCmd(uint8_t subcmd, uint8_t * data)
{
    uint8_t len = 5;

    switch(subcmd)
    {
        case MDB_VEND_REQ_SUBCMD:
            {
                if (mdb_dev.is_expansion_en &&
                    mdb_dev.level == MDB_LEVEL_3)
                {
                    len = 6;
                }
                break;
            }
        case MDB_VEND_SESS_COMPL_SUBCMD:
        case MDB_VEND_FAILURE_SUBCMD:
        case MDB_VEND_CANCEL_SUBCMD:
            {
                len = 0;
                break;
            }
        case MDB_VEND_SUCCESS_SUBCMD:
            {
                len = 2;
                break;
            }
        case MDB_VEND_CASH_SALE_SUBCMD:
            {
                if (mdb_dev.is_expansion_en &&
                    mdb_dev.level == MDB_LEVEL_3)
                {
                    len = 8;
                }
                break;
            }
        case MDB_VEND_NEG_REQ_SUBCMD:
            {
                if (mdb_dev.is_expansion_en &&
                    mdb_dev.level == MDB_LEVEL_3)
                {
                    len = 8;
                }
                break;
            }
        default: break;
    }

    MdbSendCmd(MDB_VEND_CMD, subcmd, data, len);
}

void MdbReaderCmd(uint8_t subcmd, uint8_t * data)
{
    uint8_t len = 1;
    if (subcmd == MDB_READER_DATA_RESP_SUBCMD &&
        mdb_dev.level == MDB_LEVEL_3)
    {
        len = 8;
    }

    MdbSendCmd(MDB_READER_CMD, subcmd, data, len);
}

void MdbRevalueCmd(uint8_t subcmd, uint8_t * data)
{
    uint8_t len = 2;

    if (mdb_dev.level == MDB_LEVEL_1)
    {
        return;
    }

    switch(subcmd)
    {
        case MDB_REVALUE_REQ_SUBCMD:
            {
                if (subcmd == MDB_READER_DATA_RESP_SUBCMD &&
                    mdb_dev.level == MDB_LEVEL_3)
                {
                    len = 4;
                }
                break;
            }
        case MDB_REVALUE_LIM_REQ_SUBCMD:
            {
                len = 0;
                break;
            }
        default: break;
    }

    MdbSendCmd(MDB_REVALUE_CMD, subcmd, data, len);
}

void MdbExpansionCmd(uint8_t subcmd, uint8_t * data)
{
    uint8_t len = 0;

    switch(subcmd)
    {
        case MDB_EXP_REQ_ID_SUBCMD:
            {
                len = 30;
                break;
            }
        case MDB_EXP_W_TIME_DATA_SUBCMD:
            {
                if (mdb_dev.level == 2 ||
                    mdb_dev.level == 3)
                {
                    len = 11;
                }
                break;
            }
        case MDB_EXP_OPT_FTR_EN_SUBCMD:
            {
                if (mdb_dev.level == 3)
                {
                    len = 5;
                }
            }
        default: break;
    }

    MdbSendCmd(MDB_EXPANSION_CMD, subcmd, data, len);
}

void MdbAckCmd(void)
{
    mdb_dev.tx_data[0] = MDB_ACK;
    MdbSendData(mdb_dev.tx_data, 1);
}

static void MdbSendCmd(uint8_t cmd, uint8_t subcmd, uint8_t * data, uint8_t len)
{
    mdb_dev.send_cmd    = cmd;
    mdb_dev.send_subcmd = subcmd;

    mdb_dev.tx_data[0] = cmd + mdb_dev.addr + MDB_MODE_BIT;

    if (len)
    {
        mdb_dev.tx_data[1] = subcmd;
    }

    if (data != NULL)
    {
        for(uint8_t i = 1; i < len; i++)
        {
            mdb_dev.tx_data[i + 1] = (uint16_t)(data[i - 1] & 0x00FF);
        }
    }

    if (len > 1)
    {
        mdb_dev.tx_data[len + 1] = MdbCalcChk(mdb_dev.tx_data, len + 1);
        MdbSendData(mdb_dev.tx_data, len + 2);
    }
    else
    {
        mdb_dev.tx_data[len + 1] = MdbCalcChk(mdb_dev.tx_data, len + 1);
        MdbSendData(mdb_dev.tx_data, len + 2);
    }

}

// static void MdbSendResponce(uint8_t resp)
// {
//     mdb_dev.tx_data[0] = resp;
//     MdbSendData(mdb_dev.tx_data, 1);
// }

static uint16_t MdbCalcChk(uint16_t * buf, uint8_t len)
{
    uint16_t check_sum = 0;

    for(uint8_t i = 0; i < len; i++)
    {
        check_sum += buf[i] & 0xFF;
    }

    return check_sum & 0xFF;
}

static bool MdbIsValidateChk(uint16_t * buf, uint8_t len)
{
    uint16_t calc_sum = 0;
    uint16_t rec_sum  = buf[len - 1] & 0xFF;

    for(uint8_t i = 0; i < len - 1; i++)
    {
        calc_sum += buf[i] & 0xFF;
    }

    calc_sum = calc_sum & 0xFF;

    return (calc_sum == rec_sum) ? true : false;
}

static void MdbSendData(uint16_t * buf, uint8_t len)
{
    mdb_dev.send_callback(buf, len);
}

uint16_t MdbGetRxCh(uint8_t idx)
{
    if (idx > MDB_MAX_BUF_LEN - 1)
        return 0;

    return mdb_dev.rx_data[idx];
}

// void MdbClearRx(uint8_t idx)
// {
//     mdb_dev.rx_data[idx] &= ~0x100;
// }

// static void MdbReceiveData(uint16_t * buf, uint8_t len)
// {
//     if (len > MDB_MAX_BUF_LEN)
//         return;

//     mdb_dev.rx_len = len;
//     memcpy(mdb_dev.rx_data, buf, len);
// }

mdb_ret_resp_t MdbReceiveChar(uint16_t ch)
{
    mdb_ret_resp_t ret = MDB_RET_IN_PROG;

    mdb_dev.rx_data[mdb_dev.rx_len] = ch;
    mdb_dev.rx_len++;

    if (ch & 0x100)
    {
        ret = MdbParseData(mdb_dev.rx_len);
        mdb_dev.rx_len = 0;
    }

    return ret;
}

static mdb_ret_resp_t MdbParseData(uint8_t len)
{
    mdb_ret_resp_t ret = MDB_RET_IDLE;

    mdb_dev.rx_len = len;
    if (mdb_dev.rx_len == 1)
    {
        switch(mdb_dev.rx_data[0] & 0xFF)
        {
            case MDB_ACK:
                ret = MDB_RET_IDLE;
                break;
            case MDB_NAK:
                ret = MDB_RET_IDLE;
                break;
            default:
                ret = MDB_RET_IDLE;
        }
        return ret;
    }

    if (MdbIsValidateChk(mdb_dev.rx_data, mdb_dev.rx_len))
    {
        MdbParseResponse();
        return MDB_RET_DATA;
    }
    else
    {
        return MDB_RET_REPEAT;
    }
}

static void MdbParseResponse(void)
{
    uint8_t resp_header = mdb_dev.rx_data[0];

    switch(resp_header)
    {
        case MDB_POLL_JUST_RESET_RESP:
            {
                mdb_dev.state = MDB_STATE_DISABLED;
                break;
            }
        case MDB_POLL_CONFIG_RESP:
            {
                mdb_dev.dev_slave.country_code = (mdb_dev.rx_data[2] << 8) + mdb_dev.rx_data[3];
                if (mdb_dev.dev_slave.country_code != MDB_CURRENCY_CODE_RUB_1 ||
                    mdb_dev.dev_slave.country_code != MDB_CURRENCY_CODE_RUB_1)
                {
                    mdb_dev.state = MDB_STATE_INACTIVE;
                    return;
                }
                mdb_dev.dev_slave.level             = mdb_dev.rx_data[1];
                mdb_dev.dev_slave.scale_factor      = mdb_dev.rx_data[4];
                mdb_dev.dev_slave.decimal_places    = mdb_dev.rx_data[5];
                mdb_dev.dev_slave.max_resp_time     = mdb_dev.rx_data[6];
                mdb_dev.dev_slave.misc              = mdb_dev.rx_data[7];

                if (mdb_dev.level > mdb_dev.dev_slave.level)
                {
                    MdbSetSlaveAddr(mdb_dev.dev_slave.level);
                }

                mdb_dev.state = MDB_STATE_ENABLED;
                break;
            }
        case MDB_POLL_DISPLAY_REQ_RESP:
            break;
        case MDB_POLL_BEGIN_SESSION_RESP:
            {
                mdb_dev.state = MDB_STATE_SESSION_IDLE;
                MdbSelectItem();
                break;
            }
        case MDB_POLL_SESS_CANCEL_RESP:
            {
                MdbSessionCancel();
                break;
            }
        case MDB_POLL_VEND_APPROVED_RESP:
            {
                MdbVendApproved();
                break;
            }
        case MDB_POLL_VEND_DENIED_RESP:
            {
                MdbVendDenied();
                break;
            }
        case MDB_POLL_END_SESSION_RESP:
            {
                mdb_dev.state = MDB_STATE_ENABLED;
                //TODO
                // Check max non-response time!
                break;
            }
        case MDB_POLL_CANCELLED_RESP:
            {
                break;
            }
        case MDB_POLL_PERIPH_ID_RESP:
            {
                for (uint8_t i = 0; i < 3; i++)
                    mdb_dev.dev_slave.manufact_code[i] = mdb_dev.rx_data[i + 1];
                for (uint8_t i = 0; i < 12; i++)
                    mdb_dev.dev_slave.serial_num[i] = mdb_dev.rx_data[i + 1 + 3];
                for (uint8_t i = 0; i < 12; i++)
                    mdb_dev.dev_slave.model_num[i] = mdb_dev.rx_data[i + 1 + 3 + 12];
                mdb_dev.dev_slave.sw_version = (mdb_dev.rx_data[28] << 8) + mdb_dev.rx_data[29];
                break;
            }
    }
}

static void MdbSelectItem(void)
{
    if (mdb_dev.select_item_cb == NULL)
        return;

    mdb_dev.select_item_cb();
}

static void MdbSessionCancel(void)
{
    if (mdb_dev.session_cancel_cb == NULL)
        return;

    mdb_dev.session_cancel_cb();
}

static void MdbVendApproved(void)
{
    if (mdb_dev.vend_approved_cb == NULL)
        return;

    mdb_dev.vend_approved_cb();
}

static void MdbVendDenied(void)
{
    if (mdb_dev.vend_denied_cb == NULL)
        return;

    mdb_dev.vend_denied_cb();
}

mdb_level_t MdbGetLevel(void)
{
    return mdb_dev.level;
}

void MdbUsartInit(void)
{
    USART_InitTypeDef USART_InitStructure;
    GPIO_InitTypeDef  GPIO_InitStructure;
    DMA_InitTypeDef   DMA_InitStructure;
    NVIC_InitTypeDef  NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,  ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    RCC_AHBPeriphClockCmd ( RCC_AHBPeriph_DMA1,   ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure );

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // Настройка DMA

    // DMA_DeInit(DMA1_Channel6);
    // DMA_InitStructure.DMA_PeripheralBaseAddr    = (uint32_t)&USART2->DR;
    // DMA_InitStructure.DMA_MemoryBaseAddr        = (uint32_t) mdb_dev.rx_data;
    // DMA_InitStructure.DMA_DIR                   = DMA_DIR_PeripheralSRC;
    // DMA_InitStructure.DMA_BufferSize            = MDB_MAX_BUF_LEN;
    // DMA_InitStructure.DMA_PeripheralInc         = DMA_PeripheralInc_Disable;
    // DMA_InitStructure.DMA_MemoryInc             = DMA_MemoryInc_Enable;
    // DMA_InitStructure.DMA_PeripheralDataSize    = DMA_PeripheralDataSize_HalfWord;
    // DMA_InitStructure.DMA_MemoryDataSize        = DMA_MemoryDataSize_HalfWord;
    // DMA_InitStructure.DMA_Mode                  = DMA_Mode_Circular;
    // DMA_InitStructure.DMA_Priority              = DMA_Priority_High;
    // DMA_InitStructure.DMA_M2M                   = DMA_M2M_Disable;
    // DMA_Init(DMA1_Channel6, &DMA_InitStructure);

    DMA_DeInit(DMA1_Channel7);
    DMA_InitStructure.DMA_PeripheralBaseAddr    = (uint32_t)&USART2->DR;
    DMA_InitStructure.DMA_MemoryBaseAddr        = (uint32_t) mdb_dev.tx_data;
    DMA_InitStructure.DMA_DIR                   = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_BufferSize            = MDB_MAX_BUF_LEN;
    DMA_InitStructure.DMA_PeripheralInc         = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc             = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize    = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize        = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode                  = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority              = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M                   = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel7, &DMA_InitStructure);

    // DMA_Cmd(DMA1_Channel6, ENABLE);
    DMA_Cmd(DMA1_Channel7, DISABLE);

    // USART_DMACmd(USART2, USART_DMAReq_Rx, ENABLE);
    USART_DMACmd(USART2, USART_DMAReq_Tx, DISABLE);

    USART_InitStructure.USART_BaudRate = 9600;
    USART_InitStructure.USART_WordLength = USART_WordLength_9b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No ;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART2, &USART_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel7_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = configLIBRARY_LOWEST_INTERRUPT_PRIORITY;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    DMA_ITConfig(DMA1_Channel7, DMA_IT_TC, ENABLE);

    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = configLIBRARY_LOWEST_INTERRUPT_PRIORITY;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(USART2, ENABLE);
}

void MdbPrint(void)
{
    // char buf[128];
    // sprintf(buf, "\r\n===\r\n");
    // UsartDebugSendString(buf);
    // for(int i = 0; i < 20; i++)
    // {
    //  sprintf(buf, "%d ", ubuf[i]);
    //  UsartDebugSendString(buf);
    // }
}
