#ifndef _MDB_H
#define _MDB_H

#include <stdint.h>

#define MDB_VMC_MANUFACTURE_CODE    "ABC"

#define MDB_MAX_BUF_LEN             36

#define MDB_POLL_TIME               125     /* 25-200 ms. Recmmended 125-200 ms. */
#define MDB_T_RESPONSE_TIMEOUT      5       /* ms */
#define MDB_NON_RESP_TIMEOUT        5       /* s, by default */
#define MDB_T_RESET_TIMEOUT         10      /* s */
#define MDB_COUNT_NON_RESP          10

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
#define MDB_READER_DATA_RESP_SUBCMD 0x03
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

typedef enum
{
    MDB_LEVEL_1 = 1,
    MDB_LEVEL_2,
    MDB_LEVEL_3
} mdb_level_t;

typedef enum
{
    MDB_RET_DATA,
    MDB_RET_IN_PROG,
    MDB_RET_IDLE,
    MDB_RET_REPEAT
} mdb_ret_resp_t;

typedef struct
{
    void            (*send_callback)(const uint16_t*, uint8_t);
    void            (*select_item_cb)(void);
    void            (*session_cancel_cb)(void);
    void            (*vend_approved_cb)(void);
    void            (*vend_denied_cb)(void);
    void            (*update_resp_time_cb)(uint8_t);
} mdv_dev_init_struct_t;
//void MdbPrint(void);
//void MdbBufSend(const uint16_t *pucBuffer, uint8_t len);

void MdbInit(mdv_dev_init_struct_t dev_struct);
void MdbSetSlaveAddr(uint8_t addr);
void MdbResetCmd(void);
void MdbSetupCmd(uint8_t subcmd, uint8_t * data);
void MdbPollCmd(void);
void MdbVendCmd(uint8_t subcmd, uint8_t * data);
void MdbReaderCmd(uint8_t subcmd, uint8_t * data);
void MdbRevalueCmd(uint8_t subcmd, uint8_t * data);
void MdbExpansionCmd(uint8_t subcmd, uint8_t * data);
void MdbAckCmd(void);
mdb_level_t MdbGetLevel(void);
uint8_t MdbGetNonRespTime(void);
// void MdbGetCh(uint16_t ch);
void MdbUsartInit(void);
uint16_t MdbGetRxCh(uint8_t idx);
// void MdbClearRx(uint8_t idx);
mdb_ret_resp_t MdbReceiveChar(uint16_t ch);

#endif /* _MDB_H */