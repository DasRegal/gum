#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#ifndef TEST_MDB
#include "stm32f10x.h"
#include "FreeRTOS.h"
#endif

#include "mdb.h"

#ifdef TEST_MDB
# define DEBUG_PRINT(x) printf x
#else
# define DEBUG_PRINT(x) do {} while (0)
#endif

#define MDB_CASHLESS_DEV_1_ADDR     0x10
#define MDB_CASHLESS_DEV_2_ADDR     0x60

#define MDB_ACK                     0x00
#define MDB_RET                     0xAA
#define MDB_NAK                     0xFF
#define MDB_CMD_OUT_OF_SEQUENCE     0x0B

#define MDB_MODE_BIT                0x100

/* VMC LEVELS SUPPORT */
#define MDB_LVL_SUP_1               1
#define MDB_LVL_SUP_2               2
#define MDB_LVL_SUP_3               4
#define MDB_LVL_SUP_EXT_CUR         8
#define MDB_LVL_SUP_ALL             (MDB_LVL_SUP_1 | MDB_LVL_SUP_2 | MDB_LVL_SUP_3)
#define MDB_LVL_SUP_ALL_AND_EXT_CUR (MDB_LVL_SUP_ALL | MDB_LVL_SUP_EXT_CUR)

#define MDB_RESET_CMD               0
#define MDB_SETUP_CMD               1
#define MDB_POLL_CMD                2
#define MDB_VEND_CMD                3
#define MDB_READER_CMD              4
#define MDB_REVALUE_CMD             5
#define MDB_EXPANSION_CMD           7

/* MDB_SETUP_CMD */
#define MDB_SETUP_CONF_DATA_SUBCMD  0x00
#define MDB_SETUP_PRICE_SUBCMD      0x01
/* MDB_VEND_CMD */
#define MDB_VEND_REQ_SUBCMD         0x00
#define MDB_VEND_CANCEL_SUBCMD      0x01
#define MDB_VEND_SUCCESS_SUBCMD     0x02
#define MDB_VEND_FAILURE_SUBCMD     0x03
#define MDB_VEND_SESS_COMPL_SUBCMD  0x04
#define MDB_VEND_CASH_SALE_SUBCMD   0x05
#define MDB_VEND_NEG_REQ_SUBCMD     0x06
/* MDB_READER_CMD */
#define MDB_READER_DISABLE_SUBCMD   0x00
#define MDB_READER_ENABLE_SUBCMD    0x01
#define MDB_READER_CANCEL_SUBCMD    0x02
/* MDB_REVALUE_CMD */
#define MDB_REVALUE_REQ_SUBCMD      0x00
#define MDB_REVALUE_LIM_REQ_SUBCMD  0x01
/* MDB_EXPANSION_CMD */
#define MDB_EXP_REQ_ID_SUBCMD       0x00
#define MDB_EXP_READ_FILE_SUBCMD    0x01
#define MDB_EXP_WRITE_FILE_SUBCMD   0x02
#define MDB_EXP_W_TIME_DATA_SUBCMD  0x03
#define MDB_EXP_OPT_FTR_EN_SUBCMD   0x04
#define MDB_EXP_FTL_RX_REQ_SUBCMD   0xFA
#define MDB_EXP_FTL_RET_DEN_SUBCMD  0xFB
#define MDB_EXP_FTL_TX_BLK_SUBCMD   0xFC
#define MDB_EXP_FTL_TX_OK_SUBCMD    0xFD
#define MDB_EXP_FTL_TX_REQ_SUBCMD   0xFE
#define MDB_EXP_DIAGNOSTICS_SUBCMD  0xFF

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

#define MDB_PAY_MEDIA_ERR           0x00
#define MDB_INVLD_PAY_MEDIA_ERR     0x01
#define MDB_TAMPER_ERR              0x02
#define MDB_MANUF_DEF_1_ERR         0x03
#define MDB_COMMUNICATIONS_2_ERR    0x04
#define MDB_READER_REQ_SERVICE_ERR  0x05
#define MDB_UNASSIGNED_ERR          0x06
#define MDB_MANUF_DEF_2_ERR         0x07
#define MDB_READER_FAILURE_ERR      0x08
#define MDB_COMMUNICATIONS_3_ERR    0x09
#define MDB_PAY_MEDIA_JAMMED_ERR    0x0A
#define MDB_MANUF_DEF_ERR           0x0B
#define MDB_REFUND_ERR              0x0C

#define MDB_CURRENCY_CODE_RUB_1     0x643
#define MDB_CURRENCY_CODE_RUB_2     0x810

// extern void UsartDebugSendString(const char *pucBuffer);

static void MdbSendCmd(uint8_t cmd, uint8_t subcmd, uint8_t * data, uint8_t len);
static uint16_t MdbCalcChk(uint16_t * buf, uint8_t len);
static bool MdbIsValidateChk(uint16_t * buf, uint8_t len);
static void MdbSendData(uint16_t * buf, uint8_t len);
static mdb_ret_resp_t MdbParseData(uint8_t len);
static mdb_ret_resp_t MdbParseResponse(void);
static void MdbSelectItem(void);
static void MdbSessionCancel(void);

typedef struct
{
    uint8_t cmd_data;
    uint8_t data_size[4];   /* For each level + External Currency*/
    uint8_t is_level_bit;
} mdb_data_t;

typedef struct
{
    mdb_code_cmd_t  code_cmd;
    mdb_data_t      response;
} mdb_resp_data_t;

typedef struct
{
          mdb_code_cmd_t   code_cmd;
          mdb_data_t      *sub_cmd;
    const mdb_resp_data_t *response;
} mdb_cmd_t;

const mdb_resp_data_t poll_response[18] = 
{
    { 0 , { MDB_POLL_JUST_RESET_RESP,    { 1,  1,  1,  1 }, MDB_LVL_SUP_ALL_AND_EXT_CUR                         } },
    { 0 , { MDB_POLL_CONFIG_RESP,        { 8,  8,  8,  8 }, MDB_LVL_SUP_ALL_AND_EXT_CUR                         } },
    { 0 , { MDB_POLL_DISPLAY_REQ_RESP,   { 34, 34, 34, 34}, MDB_LVL_SUP_ALL_AND_EXT_CUR                         } },
    { 0 , { MDB_POLL_BEGIN_SESSION_RESP, { 3,  10, 10, 17}, MDB_LVL_SUP_ALL_AND_EXT_CUR                         } },
    { 0 , { MDB_POLL_SESS_CANCEL_RESP,   { 1,  1,  1,  1 }, MDB_LVL_SUP_ALL_AND_EXT_CUR                         } },
    { 0 , { MDB_POLL_VEND_APPROVED_RESP, { 3,  3,  3,  5 }, MDB_LVL_SUP_ALL_AND_EXT_CUR                         } },
    { 0 , { MDB_POLL_VEND_DENIED_RESP,   { 1,  1,  1,  1 }, MDB_LVL_SUP_ALL_AND_EXT_CUR                         } },
    { 0 , { MDB_POLL_END_SESSION_RESP,   { 1,  1,  1,  1 }, MDB_LVL_SUP_ALL_AND_EXT_CUR                         } },
    { 0 , { MDB_POLL_CANCELLED_RESP,     { 1,  1,  1,  1 }, MDB_LVL_SUP_ALL_AND_EXT_CUR                         } },
    { 0 , { MDB_POLL_PERIPH_ID_RESP,     { 30, 30, 34, 34}, MDB_LVL_SUP_ALL_AND_EXT_CUR                         } },
    { 0 , { MDB_POLL_ERROR_RESP,         { 2,  2,  2,  2 }, MDB_LVL_SUP_ALL_AND_EXT_CUR                         } },
    { 0 , { MDB_POLL_OUT_OF_SEQ_RESP,    { 1,  2,  2,  2 }, MDB_LVL_SUP_ALL_AND_EXT_CUR                         } },
    { 0 },
    { 0 , { MDB_POLL_REVAL_APPR_RESP,    { 0,  1,  1,  1 }, MDB_LVL_SUP_2 | MDB_LVL_SUP_3 | MDB_LVL_SUP_EXT_CUR } },
    { 0 , { MDB_POLL_REVAL_DENIED_RESP,  { 0,  1,  1,  1 }, MDB_LVL_SUP_2 | MDB_LVL_SUP_3 | MDB_LVL_SUP_EXT_CUR } },
    { 0 , { MDB_POLL_REVAL_LIMIT_RESP,   { 0,  3,  3,  5 }, MDB_LVL_SUP_2 | MDB_LVL_SUP_3 | MDB_LVL_SUP_EXT_CUR } },
    { 0 },
    { 0 , { MDB_POLL_TIME_DATA_RESP,     { 0,  1,  1,  1 }, MDB_LVL_SUP_2 | MDB_LVL_SUP_3 | MDB_LVL_SUP_EXT_CUR } }
};

mdb_data_t setup_sub_cmd[2] = 
{
    { MDB_SETUP_CONF_DATA_SUBCMD, { 5, 5, 5, 5 }, MDB_LVL_SUP_ALL_AND_EXT_CUR },
    { MDB_SETUP_PRICE_SUBCMD    , { 5, 5, 5, 11}, MDB_LVL_SUP_ALL_AND_EXT_CUR }
};

const mdb_resp_data_t setup_response[2] = 
{
    poll_response[MDB_POLL_CONFIG_RESP],
    { 0 },
};

mdb_data_t vend_sub_cmd[7] = 
{
    { MDB_VEND_REQ_SUBCMD,         { 5,  5,  5,  7 }, MDB_LVL_SUP_ALL_AND_EXT_CUR         },
    { MDB_VEND_CANCEL_SUBCMD,      { 1,  1,  1,  1 }, MDB_LVL_SUP_ALL_AND_EXT_CUR         },
    { MDB_VEND_SUCCESS_SUBCMD,     { 3,  3,  3,  3 }, MDB_LVL_SUP_ALL_AND_EXT_CUR         },
    { MDB_VEND_FAILURE_SUBCMD,     { 1,  1,  1,  1 }, MDB_LVL_SUP_ALL_AND_EXT_CUR         },
    { MDB_VEND_SESS_COMPL_SUBCMD,  { 1,  1,  1,  1 }, MDB_LVL_SUP_ALL_AND_EXT_CUR         },
    { MDB_VEND_CASH_SALE_SUBCMD,   { 5,  5,  5,  9 }, MDB_LVL_SUP_ALL_AND_EXT_CUR         },
    { MDB_VEND_NEG_REQ_SUBCMD,     { 0,  0,  5,  7 }, MDB_LVL_SUP_3 | MDB_LVL_SUP_EXT_CUR },
};

mdb_resp_data_t vend_response[9] =
{
    poll_response[MDB_POLL_VEND_APPROVED_RESP],
    poll_response[MDB_POLL_VEND_DENIED_RESP],
    poll_response[MDB_POLL_VEND_DENIED_RESP],
    { 0 },
    { 0 },
    poll_response[MDB_POLL_END_SESSION_RESP],
    { 0 },
    poll_response[MDB_POLL_VEND_APPROVED_RESP],
    poll_response[MDB_POLL_VEND_DENIED_RESP]
};

mdb_data_t reader_sub_cmd[3] =
{
    { MDB_READER_DISABLE_SUBCMD, { 1,  1,  1,  1 }, MDB_LVL_SUP_ALL_AND_EXT_CUR },
    { MDB_READER_ENABLE_SUBCMD,  { 1,  1,  1,  1 }, MDB_LVL_SUP_ALL_AND_EXT_CUR },
    { MDB_READER_CANCEL_SUBCMD,  { 1,  1,  1,  1 }, MDB_LVL_SUP_ALL_AND_EXT_CUR }
};

mdb_resp_data_t reader_response[3] =
{
    { 0 },
    { 0 },
    poll_response[MDB_POLL_CANCELLED_RESP]
};

mdb_data_t revalue_sub_cmd[2] =
{
    { MDB_REVALUE_REQ_SUBCMD,     { 0, 3, 3, 5}, MDB_LVL_SUP_2 | MDB_LVL_SUP_3 | MDB_LVL_SUP_EXT_CUR },
    { MDB_REVALUE_LIM_REQ_SUBCMD, { 0, 1, 1, 1}, MDB_LVL_SUP_2 | MDB_LVL_SUP_3 | MDB_LVL_SUP_EXT_CUR }
};

mdb_resp_data_t revalue_response[3] =
{
    poll_response[MDB_POLL_REVAL_APPR_RESP]  ,
    poll_response[MDB_POLL_REVAL_DENIED_RESP],
    poll_response[MDB_POLL_REVAL_LIMIT_RESP]
};

mdb_data_t expansion_sub_cmd[5] =
{
    { MDB_EXP_REQ_ID_SUBCMD,      { 30, 30, 30, 30 }, MDB_LVL_SUP_ALL_AND_EXT_CUR         },
    { MDB_EXP_READ_FILE_SUBCMD,   { 0 },              0                                   },
    { MDB_EXP_WRITE_FILE_SUBCMD,  { 0 },              0                                   },
    { MDB_EXP_W_TIME_DATA_SUBCMD, { 0 },              0                                   },
    { MDB_EXP_OPT_FTR_EN_SUBCMD,  { 0,  0,  5,  5  }, MDB_LVL_SUP_3 | MDB_LVL_SUP_EXT_CUR }
};

mdb_resp_data_t expansion_response[2] =
{
    poll_response[MDB_POLL_PERIPH_ID_RESP],
    { 0 }
};

mdb_cmd_t mdb_cmd[8] = 
{
    { MDB_RESET_CMD_E,     NULL,              NULL               },
    { MDB_SETUP_CMD_E,     setup_sub_cmd,     setup_response     },
    { MDB_POLL_CMD_E,      NULL,              poll_response      },
    { MDB_VEND_CMD_E,      vend_sub_cmd,      vend_response      },
    { MDB_READER_CMD_E,    reader_sub_cmd,    reader_response    },
    { MDB_REVALUE_CMD_E,   revalue_sub_cmd,   revalue_response   },
    { 0 },
    { MDB_EXPANSION_CMD_E, expansion_sub_cmd, expansion_response },
};

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
    mdb_dev.rx_len          = 0;
    mdb_dev.level           = MDB_LEVEL_2;
    mdb_dev.state           = MDB_STATE_INACTIVE;
    mdb_dev.addr            = MDB_CASHLESS_DEV_1_ADDR;
    mdb_dev.dev_slave.max_resp_time = 0xFF;
}

void MdbSetSlaveAddr(uint8_t addr)
{
    mdb_dev.addr = addr;
}

/**
  * @brief  Setup reader
  * @param  cmd:
  *                 MDB_RESET_CMD_E
  *                 MDB_SETUP_CMD_E
  *                 MDB_POLL_CMD_E
  *                 MDB_VEND_CMD_E
  *                 MDB_READER_CMD_E
  *                 MDB_REVALUE_CMD_E
  *                 MDB_EXPANSION_CMD_E
*/
void MdbSendCommand(uint8_t cmd, uint8_t subcmd, uint8_t * data)
{
    uint8_t size = 0;
    uint8_t lvl_type = 0;
    uint8_t lvl_support = 0;

    mdb_dev.send_cmd    = cmd;
    mdb_dev.send_subcmd = subcmd;

    mdb_dev.tx_data[0] = cmd + mdb_dev.addr + MDB_MODE_BIT;

    if (mdb_cmd[cmd].sub_cmd == NULL)
    {
        mdb_dev.tx_data[1] = MdbCalcChk(mdb_dev.tx_data, 1);
        MdbSendData(mdb_dev.tx_data, 2);
        // DEBUG_PRINT(("\n%s: 0x%x 0x%x\n", __func__, mdb_dev.tx_data[0], mdb_dev.tx_data[1]));
        return;
    }

    lvl_support = mdb_cmd[cmd].sub_cmd[subcmd].is_level_bit;

    if(!(lvl_support & (1 << (mdb_dev.level - 1))))
    {
        DEBUG_PRINT(("Command not supported for level %d!", mdb_dev.level));
        return; /* Command not support */
    }

    if (mdb_dev.is_expansion_en)
        lvl_type = 3;
    else
        lvl_type = mdb_dev.level - 1;

    size = mdb_cmd[cmd].sub_cmd[subcmd].data_size[lvl_type];

    mdb_dev.tx_data[0] = cmd + mdb_dev.addr + MDB_MODE_BIT;

    if (size)
    {
        mdb_dev.tx_data[1] = subcmd;
    }

    if (data != NULL)
    {
        for(uint8_t i = 1; i < size; i++)
        {
            mdb_dev.tx_data[i + 1] = (uint16_t)(data[i - 1] & 0x00FF);
        }
    }

    if (size > 1)
    {
        mdb_dev.tx_data[size + 1] = MdbCalcChk(mdb_dev.tx_data, size + 1);
        MdbSendData(mdb_dev.tx_data, size + 2);
    }
    else
    {
        mdb_dev.tx_data[size + 1] = MdbCalcChk(mdb_dev.tx_data, size + 1);
        MdbSendData(mdb_dev.tx_data, size + 2);
    }

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

mdb_ret_resp_t MdbReceiveChar(uint16_t ch)
{
    mdb_ret_resp_t ret = MDB_RET_IN_PROGRESS;

    mdb_dev.rx_data[mdb_dev.rx_len] = ch;

    if (mdb_dev.rx_len > MDB_MAX_BUF_LEN)
    {
        mdb_dev.rx_len = 0;
        mdb_dev.rx_data[0] = 0;
        return ret;
    }

    mdb_dev.rx_len++;

    if (ch & 0x100)
    {
        ret = MdbParseData(mdb_dev.rx_len);
        mdb_dev.rx_len = 0;
        mdb_dev.rx_data[0] = 0;
    }

    return ret;
}

static mdb_ret_resp_t MdbParseData(uint8_t len)
{
    mdb_ret_resp_t ret = MDB_RET_IDLE;

    if (mdb_dev.rx_len == 1)
    {
        switch(mdb_dev.rx_data[0] & 0xFF)
        {
            case MDB_ACK:

                if(mdb_dev.send_cmd == MDB_VEND_CMD_E)
                {
                    switch(mdb_dev.send_subcmd)
                    {
                        case MDB_VEND_REQ_SUBCMD:
                            mdb_dev.state = MDB_STATE_VEND;
                            break;
                        case MDB_VEND_SUCCESS_SUBCMD:
                        case MDB_VEND_CANCEL_SUBCMD:
                            mdb_dev.state = MDB_STATE_SESSION_IDLE;
                            break;
                        case MDB_VEND_NEG_REQ_SUBCMD:
                            mdb_dev.state = MDB_STATE_NEG_VEND;
                            break;
                        default:
                            break;
                    }
                }

                if(mdb_dev.send_cmd == MDB_RESET_CMD_E)
                    mdb_dev.state = MDB_STATE_INACTIVE;

                ret = MDB_RET_ACK;
                break;
            case MDB_NAK:
                ret = MDB_RET_IDLE;
                ret = MDB_RET_NACK;
                break;
            default:
                ret = MDB_RET_IDLE;
                ret = MDB_RET_NACK;
        }
        return ret;
    }

    if (MdbIsValidateChk(mdb_dev.rx_data, mdb_dev.rx_len))
    {
        ret = MdbParseResponse();
        return ret;
    }
    else
    {
        return MDB_RET_REPEAT;
    }
}

static mdb_ret_resp_t MdbParseResponse(void)
{
    uint8_t resp_header = mdb_dev.rx_data[0];

    switch(resp_header)
    {
        /* Not resp, only POLL */
        case MDB_POLL_JUST_RESET_RESP:
            {
                mdb_dev.state = MDB_STATE_INACTIVE;
                return MDB_RET_JUST_RESET;
                return MDB_RET_OK_DATA;
            }
        /* Resp & POLL */
        case MDB_POLL_CONFIG_RESP:
            {
                mdb_dev.state = MDB_STATE_DISABLED;

                mdb_dev.dev_slave.country_code = (mdb_dev.rx_data[2] << 8) + mdb_dev.rx_data[3];
                if (mdb_dev.dev_slave.country_code != MDB_CURRENCY_CODE_RUB_1 &&
                    mdb_dev.dev_slave.country_code != MDB_CURRENCY_CODE_RUB_2)
                {
                    return MDB_RET_ERROR_CURRENCY_CODE;
                }

                mdb_dev.dev_slave.level             = mdb_dev.rx_data[1];
                mdb_dev.dev_slave.scale_factor      = mdb_dev.rx_data[4];
                mdb_dev.dev_slave.decimal_places    = mdb_dev.rx_data[5];
                mdb_dev.dev_slave.misc              = mdb_dev.rx_data[7];

                // if (mdb_dev.level > mdb_dev.dev_slave.level)
                // {
                //     mdb_dev.level = mdb_dev.dev_slave.level;
                //     mdb_dev.dev_slave.max_resp_time = mdb_dev.rx_data[6];
                //     return MDB_RET_UPDATE_LEVEL;
                // }

                if (mdb_dev.dev_slave.max_resp_time != mdb_dev.rx_data[6])
                {
                    mdb_dev.dev_slave.max_resp_time = mdb_dev.rx_data[6];
                    // return MDB_RET_UPDATE_RESPONSE_TIME;
                }

                return MDB_RET_CONFIG;
            }
        /* Not resp, only POLL */
        case MDB_POLL_DISPLAY_REQ_RESP:
            // TODO Проверить функционал вендисты
            return MDB_RET_OK_DATA;
        /* Not resp, only POLL */
        case MDB_POLL_BEGIN_SESSION_RESP:
            {
                mdb_dev.state = MDB_STATE_SESSION_IDLE;
                // TODO Проверить что приходит
                // MdbSelectItem();
                return MDB_RET_BEGIN_SESSION;
            }
        /* Not resp, only POLL p134 */
        case MDB_POLL_SESS_CANCEL_RESP:
            {
                return MDB_RET_SESS_CANCEL;
            }
        /* Resp & POLL */
        case MDB_POLL_VEND_APPROVED_RESP:
            {
                return MDB_RET_VEND_APPROVED;
            }
        /* Resp & POLL */
        case MDB_POLL_VEND_DENIED_RESP:
            {
                /*  Do not dispense any products */
                mdb_dev.state = MDB_STATE_SESSION_IDLE;
                return MDB_RET_VEND_DENIED;
            }
        /* Resp & POLL */
        case MDB_POLL_END_SESSION_RESP:
            {
                mdb_dev.state = MDB_STATE_ENABLED;
                //TODO
                // Check max non-response time!
                return MDB_RET_END_SESSION;
                break;
            }
        /* Resp & POLL */
        case MDB_POLL_CANCELLED_RESP:
            {
                break;
            }
        /* Resp & POLL */
        case MDB_POLL_PERIPH_ID_RESP:
            {
                for (uint8_t i = 0; i < 3; i++)
                    mdb_dev.dev_slave.manufact_code[i] = mdb_dev.rx_data[i + 1];
                for (uint8_t i = 0; i < 12; i++)
                    mdb_dev.dev_slave.serial_num[i] = mdb_dev.rx_data[i + 1 + 3];
                for (uint8_t i = 0; i < 12; i++)
                    mdb_dev.dev_slave.model_num[i] = mdb_dev.rx_data[i + 1 + 3 + 12];
                mdb_dev.dev_slave.sw_version = (mdb_dev.rx_data[28] << 8) + mdb_dev.rx_data[29];
                return MDB_RET_PERIPH_ID;
                break;
            }
        /* Not resp, only POLL */
        case MDB_POLL_ERROR_RESP:
            {
                switch(mdb_dev.rx_data[1])
                {
                    case MDB_PAY_MEDIA_ERR:
                    case MDB_INVLD_PAY_MEDIA_ERR:
                    case MDB_TAMPER_ERR:
                    case MDB_MANUF_DEF_1_ERR:
                    case MDB_COMMUNICATIONS_2_ERR:
                    case MDB_READER_REQ_SERVICE_ERR:
                    case MDB_UNASSIGNED_ERR:
                    case MDB_MANUF_DEF_2_ERR:
                    case MDB_READER_FAILURE_ERR:
                    case MDB_COMMUNICATIONS_3_ERR:
                    case MDB_PAY_MEDIA_JAMMED_ERR:
                    case MDB_MANUF_DEF_ERR:
                    case MDB_REFUND_ERR:
                        break;
                    default: break;
                }
                break;
            }
        /* Not resp, only POLL */
        case MDB_POLL_OUT_OF_SEQ_RESP:
            {
                mdb_dev.state = MDB_STATE_SESSION_IDLE;
                switch(mdb_dev.rx_data[1])
                {
                    case MDB_STATE_INACTIVE:
                    case MDB_STATE_DISABLED:
                    case MDB_STATE_ENABLED:
                    case MDB_STATE_SESSION_IDLE:
                    case MDB_STATE_VEND:
                    case MDB_STATE_REVALUE:
                    case MDB_STATE_NEG_VEND:
                        /* RESET! */
                        break;
                    default: break;
                }
                break;
            }
        /* Resp & POLL */
        case MDB_POLL_REVAL_APPR_RESP:
            {
                return MDB_RET_REVALUE_APPROVED;
            }
        /* Resp & POLL */
        case MDB_POLL_REVAL_DENIED_RESP:
            {
                return MDB_RET_REVALUE_DENIED;
            }
        /* Resp & POLL */
        case MDB_POLL_REVAL_LIMIT_RESP:
            {
                break;
            }
        /* Not resp, only POLL */
        case MDB_POLL_TIME_DATA_RESP:
            {
                break;
            }

        default: break;
    }
    return MDB_RET_OK_DATA;
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

mdb_level_t MdbGetLevel(void)
{
    return mdb_dev.level;
}

uint8_t MdbGetNonRespTime(void)
{
    return mdb_dev.dev_slave.max_resp_time;
}

void MdbSetLevel(mdb_level_t level)
{
    mdb_dev.level = level;
}

void MdbSetExpansion(bool is_exp)
{
    mdb_dev.is_expansion_en = is_exp;
}

mdb_state_t MdbGetMachineState(void)
{
    return mdb_dev.state;
}

#ifndef TEST_MDB
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

    DMA_DeInit(DMA1_Channel7);
    DMA_InitStructure.DMA_PeripheralBaseAddr    = (uint32_t)&USART2->DR;
    DMA_InitStructure.DMA_MemoryBaseAddr        = (uint32_t) mdb_dev.tx_data;
    DMA_InitStructure.DMA_DIR                   = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_BufferSize            = MDB_MAX_BUF_LEN;
    DMA_InitStructure.DMA_PeripheralInc         = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc             = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize    = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize        = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode                  = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority              = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M                   = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel7, &DMA_InitStructure);

    DMA_Cmd(DMA1_Channel7, DISABLE);

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
#endif

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
