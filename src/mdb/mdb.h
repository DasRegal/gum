#ifndef _MDB_H
#define _MDB_H

#include <stdint.h>

#define MDB_VMC_MANUFACTURE_CODE        "ABC"

#define MDB_MAX_BUF_LEN                 36

#define MDB_POLL_TIME                   125     /* 25-200 ms. Recmmended 125-200 ms. */
#define MDB_T_RESPONSE_TIMEOUT          5       /* ms */
#define MDB_NON_RESP_TIMEOUT            5       /* s, by default */
#define MDB_T_RESET_TIMEOUT             10      /* s */
#define MDB_COUNT_NON_RESP              10
#define MDB_PRODUCT_SELECTION_TIMEOUT   (30 * 1000) /* ms */
#define MDB_CARD_READ_TIMEOUT           (30 * 1000) /* ms */

/* MDB_SETUP_CMD */
#define MDB_SETUP_CONF_DATA_SUBCMD      0x00
#define MDB_SETUP_PRICE_SUBCMD          0x01
/* MDB_VEND_CMD */
#define MDB_VEND_REQ_SUBCMD             0x00
#define MDB_VEND_CANCEL_SUBCMD          0x01
#define MDB_VEND_SUCCESS_SUBCMD         0x02
#define MDB_VEND_FAILURE_SUBCMD         0x03
#define MDB_VEND_SESS_COMPL_SUBCMD      0x04
#define MDB_VEND_CASH_SALE_SUBCMD       0x05
#define MDB_VEND_NEG_REQ_SUBCMD         0x06
/* MDB_READER_CMD */
#define MDB_READER_DISABLE_SUBCMD       0x00
#define MDB_READER_ENABLE_SUBCMD        0x01
#define MDB_READER_CANCEL_SUBCMD        0x02
/* MDB_REVALUE_CMD */
#define MDB_REVALUE_REQ_SUBCMD          0x00
#define MDB_REVALUE_LIM_REQ_SUBCMD      0x01
/* MDB_EXPANSION_CMD */
#define MDB_EXP_REQ_ID_SUBCMD           0x00
#define MDB_EXP_READ_FILE_SUBCMD        0x01
#define MDB_EXP_WRITE_FILE_SUBCMD       0x02
#define MDB_EXP_W_TIME_DATA_SUBCMD      0x03
#define MDB_EXP_OPT_FTR_EN_SUBCMD       0x04
// #define MDB_EXP_FTL_RX_REQ_SUBCMD   0xFA
// #define MDB_EXP_FTL_RET_DEN_SUBCMD  0xFB
// #define MDB_EXP_FTL_TX_BLK_SUBCMD   0xFC
// #define MDB_EXP_FTL_TX_OK_SUBCMD    0xFD
// #define MDB_EXP_FTL_TX_REQ_SUBCMD   0xFE
// #define MDB_EXP_DIAGNOSTICS_SUBCMD  0xFF

typedef enum
{
    MDB_LEVEL_1 = 1,
    MDB_LEVEL_2,
    MDB_LEVEL_3,
} mdb_level_t;

typedef enum
{
    MDB_RET_OK_DATA,
    MDB_RET_IN_PROGRESS,
    MDB_RET_IDLE,
    MDB_RET_REPEAT,
    MDB_RET_UPDATE_LEVEL,
    MDB_RET_UPDATE_RESPONSE_TIME,
    MDB_RET_ERROR_CURRENCY_CODE,
    MDB_RET_NOT_ANSWER,
    MDB_RET_REVALUE_APPROVED,
    MDB_RET_REVALUE_DENIED,

    MDB_RET_ACK,
    MDB_RET_NACK,
    MDB_RET_JUST_RESET,
    MDB_RET_CONFIG,
    MDB_RET_PERIPH_ID,
    MDB_RET_BEGIN_SESSION,
    MDB_RET_SESS_CANCEL,
    MDB_RET_VEND_APPROVED,
    MDB_RET_VEND_DENIED,
    MDB_RET_END_SESSION,
} mdb_ret_resp_t;

typedef enum
{
    MDB_RESET_CMD_E     = 0,
    MDB_SETUP_CMD_E     = 1,
    MDB_POLL_CMD_E      = 2,
    MDB_VEND_CMD_E      = 3,
    MDB_READER_CMD_E    = 4,
    MDB_REVALUE_CMD_E   = 5,
    MDB_EXPANSION_CMD_E = 7
} mdb_code_cmd_t;

typedef enum
{
    MDB_STATE_INACTIVE = 1,
    MDB_STATE_DISABLED,     /* CMD_READER_ENABLE->STATE_ENABLE, CMD_RESET->STATE_INACTIVE*/
    MDB_STATE_ENABLED,      /* CMD_READER_DISABLE->STATE_DISABLE, CMD_RESET->STATE_INACTIVE */
    MDB_STATE_SESSION_IDLE, /* payment event->STATE_SESS_IDLE. CMD_SESSION_COMPLETE->?, CMD_VEND_REQUEST->STATE_VEND, CMD_REVALUE_REQUEST->STATE_REVALUE, CMD_NEG_VEND_REQUEST->STATE_NEG_VEND */
    MDB_STATE_VEND,         /* After ACK on VEND REQUEST */
    MDB_STATE_REVALUE,      /* Level 2/3 */
    MDB_STATE_NEG_VEND      /* Level 3 */
} mdb_state_t;

typedef struct
{
    void            (*send_callback)(const uint16_t*, uint8_t);
    void            (*select_item_cb)(void);
    void            (*session_cancel_cb)(void);
} mdv_dev_init_struct_t;

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
    uint16_t        amount;
} mdb_dev_t;

//void MdbPrint(void);
//void MdbBufSend(const uint16_t *pucBuffer, uint8_t len);

void MdbInit(mdv_dev_init_struct_t dev_struct);
void MdbSetSlaveAddr(uint8_t addr);
void MdbAckCmd(void);
void MdbSendCommand(uint8_t cmd, uint8_t subcmd, uint8_t * data);
mdb_level_t MdbGetLevel(void);
uint8_t MdbGetNonRespTime(void);
// void MdbGetCh(uint16_t ch);
void MdbUsartInit(void);
uint16_t MdbGetRxCh(uint8_t idx);
// void MdbClearRx(uint8_t idx);
mdb_ret_resp_t MdbReceiveChar(uint16_t ch);
mdb_state_t MdbGetMachineState(void);
uint16_t MdbGetApprovedAmount(void);
mdb_dev_t MdbGetDev(void);

#endif /* _MDB_H */