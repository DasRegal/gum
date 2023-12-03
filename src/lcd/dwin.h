#ifndef _DWIN_H
#define _DWIN_H

#include <stdbool.h>

#define DWIN_MAX_BUF_LEN 256
#define DWIN_MAX_BUTTONS 10

typedef enum
{
    DWIN_STATE_PRESTART,
    DWIN_STATE_START,
    DWIN_STATE_LEN,
    DWIN_STATE_COMMAND,
    DWIN_STATE_READ_RAM_VPL,
    DWIN_STATE_READ_RAM_VPH,
    DWIN_STATE_READ_RAM_LEN,
    DWIN_STATE_RW_RAM_DATA_L,
    DWIN_STATE_RW_RAM_DATA_H,
    DWIN_STATE_WRITE_RAM_VPL,
    DWIN_STATE_WRITE_RAM_VPH,
    DWIN_STATE_DATA,
    DWIN_STATE_FINISH
} dwin_state_t;

typedef struct
{
    bool is_pushed;
    bool is_active;
    bool arr_active[DWIN_MAX_BUTTONS];
} dwin_button_dev_t;

typedef struct
{
    dwin_state_t state;
    uint8_t     data_len;
    uint8_t     data_len_cnt;
    uint8_t     data[DWIN_MAX_BUF_LEN];
    uint8_t     cmd;
    uint16_t    vp_addr;
    uint8_t     vp_len;
    uint8_t     vp_len_cnt;
    uint16_t    vp_buf[DWIN_MAX_BUF_LEN];
    dwin_button_dev_t button_dev;
    void        (*cb_send)(const char*, uint8_t);
} dwin_dev_t;

void DwinInit(void (*cb_send)(const char*, uint8_t));
bool DwinGetCharHandler(uint8_t ch);
bool DwinIsPushButton(uint16_t *button);
void DwinHandleButton(uint16_t button);
void DwinSetPage(uint8_t page);
void DwinReset(void);
void DwinButtonEn(uint16_t button, bool isEnable);
dwin_dev_t *DwinGetDev(void);
void DwinButtonEnable(uint16_t button, bool is_enable);
void DwinAllButtonsEnable(bool is_enable);

#endif /* _DWIN_H */
