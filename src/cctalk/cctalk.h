#ifndef _CCTALK_H
#define _CCTALK_H

#include <stdbool.h>
#include <stdint.h>

#define CCTALK_MAX_BUF_LEN 256

#define CCTALK_HDR_SET_ACCEPT_LIMIT     135
#define CCTALK_HDR_MOD_INH              162
#define CCTALK_HDR_REQ_ADDR_MODE        169
#define CCTALK_HDR_REQ_BASE_YEAR        170
#define CCTALK_HDR_REQ_THERM            173
#define CCTALK_HDR_REQ_ALARM_COUNT      176
#define CCTALK_HDR_HANDHELD_FUNC        177
#define CCTALK_HDR_REQ_BANK_SEL         178
#define CCTALK_HDR_MOD_BANK_SEL         179
#define CCTALK_HDR_REQ_SECUR_SET        180
#define CCTALK_HDR_MOD_SECUR_SET        181
#define CCTALK_HDR_DL_CALIBR_INFO       182
#define CCTALK_HDR_UL_WINDOW_DATA       183
#define CCTALK_HDR_REQ_COIN_ID          184
#define CCTALK_HDR_MOD_COIN_ID          185
#define CCTALK_HDR_REQ_SORT_PATH        188
#define CCTALK_HDR_MOD_SORT_PATH        189
#define CCTALK_HDR_REQ_BUILD_CODE       192
#define CCTALK_HDR_REQ_FRAUD_CNT        193
#define CCTALK_HDR_REQ_REJ_CNT          194
#define CCTALK_HDR_REQ_LAST_MOD_DATA    195
#define CCTALK_HDR_REQ_CREATION_DATA    196
#define CCTALK_HDR_CNT_TO_EEPROM        198
#define CCTALK_HDR_CONFIG_TO_EEPROM     199
#define CCTALK_HDR_REQ_TEACH_STATUS     201
#define CCTALK_HDR_TEACH_MODE_CTRL      202
#define CCTALK_HDR_REQ_SORTER_PATH      209
#define CCTALK_HDR_MOD_SORTER_PATH      210
#define CCTALK_HDR_REQ_COIN_POS         212
#define CCTALK_HDR_REQ_OPT_FLAGS        213
#define CCTALK_HDR_WRITE_DATA_BLOCK     214
#define CCTALK_HDR_READ_DATA_BLOCK      215
#define CCTALK_HDR_EQ_DATA_STOR_AVAL    216
#define CCTALK_HDR_ENTER_PIN_NUM        218
#define CCTALK_HDR_ENTER_NEW_PIN_NUM    219
#define CCTALK_HDR_REQ_SORTER_OVRR_STAT 221
#define CCTALK_HDR_MOD_SORTER_OVRR_STAT 222
#define CCTALK_HDR_REQ_ACCEPT_COUNTER   225
#define CCTALK_HDR_REQ_INSERT_COUNTER   226
#define CCTALK_HDR_REQ_MASTER_INH_STAT  227
#define CCTALK_HDR_MOD_MASTER_INH_STAT  228
#define CCTALK_HDR_READ_BUF_CREDIT      229
#define CCTALK_HDR_REQ_INH_STAT         230
#define CCTALK_HDR_MOD_INH_STAT         231
#define CCTALK_HDR_SELF_CHECK           232 /* cctalk_self_check_err_code  */
#define CCTALK_HDR_LATCH_OUTPUT_LINES   233
#define CCTALK_HDR_READ_OPTO_STAT       236
#define CCTALK_HDR_READ_INPUT_LINES     237
#define CCTALK_HDR_TEST_OUTPUT_LINES    238
#define CCTALK_HDR_TEST_SOLENOIDS       240
#define CCTALK_HDR_REQ_SW_REV           241
#define CCTALK_HDR_REQ_SERIAL_NUM       242
#define CCTALK_HDR_REQ_DB_VERSION       243
#define CCTALK_HDR_REQ_PRODUCT_CODE     244 /* 244 + 192 */
#define CCTALK_HDR_REQ_EQ_CATEGORY_ID   245 /* cctalk_equip_cat_id */
#define CCTALK_HDR_REQ_MANUFACTURER_ID  246 /* cctalk_manufacture_id */
#define CCTALK_HDR_REQ_VARIABLE_SET     247
#define CCTALK_HDR_REQ_STATUS           248
#define CCTALK_HDR_REQ_POLLING_PRIOR    249
#define CCTALK_HDR_SIMPLE_POLL          254

typedef enum
{
    CCTALK_STATE_DST_ADDR,
    CCTALK_STATE_LEN,
    CCTALK_STATE_SRC_ADDR,
    CCTALK_STATE_HDR,
    CCTALK_STATE_DATA,
    CCTALK_STATE_CRC,
    CCTALK_STATE_FINISH
} cctalk_state_t;

typedef struct
{
    uint8_t event;
    uint8_t balance;
    bool    is_first_event;
} cctalk_credit_t;

typedef struct
{
    cctalk_state_t  state;
    char            master_addr;
    char            slave_addr;
    char            buf[CCTALK_MAX_BUF_LEN];
    char            buf_tx[CCTALK_MAX_BUF_LEN];
    uint8_t         buf_cnt;
    uint8_t         buf_data_len;
    uint8_t         crc_sum;
    void            (*send_data_cb)(const char*);
    uint32_t        sn;
    cctalk_credit_t credit;
} cctalk_master_dev_t;

typedef struct
{
    char dest_addr;
    char buf_len;
    char src_addr;
    char header;
    char *buf;
    char chk_sum;
} cctalk_data_t;

int CctalkInit(cctalk_master_dev_t dev);
bool CctalkGetCharHandler(uint8_t ch);
void CctalkSendData(uint8_t hdr, uint8_t *data, uint8_t size);
cctalk_master_dev_t *CctalkGetDev(void);
void CctalkAnswerHandle(void);
bool CctalkIsEnable(void);
void CctalkTestFunc(void);
void CoinBoxGetData(char **buf, uint8_t *len);

#endif /* _CCTALK_H */