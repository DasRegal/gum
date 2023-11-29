#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "dwin.h"

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
    DWIN_STATE_READ_RAM_DATA_L,
    DWIN_STATE_READ_RAM_DATA_H,
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
} dwin_dev_t;

dwin_dev_t dwin_dev;

void DwinButtonEnable(uint16_t button, bool is_enable)
{
    button = button & 0xFF;
    if (button > DWIN_MAX_BUTTONS)
        return;

    dwin_dev.button_dev.arr_active[button] = is_enable;
}

void DwinAllButtonsEnable(bool is_enable)
{
    for (int i = 0; i < DWIN_MAX_BUTTONS; i++)
    {
        DwinButtonEnable(i, is_enable);
    }
}

void DwinInit(void)
{
    dwin_dev.state = DWIN_STATE_PRESTART;
    dwin_dev.data_len = 0;
    dwin_dev.data_len_cnt = 0;
    dwin_dev.vp_len = 0;
    dwin_dev.vp_len_cnt = 0;
    dwin_dev.button_dev.is_pushed = 0;
    dwin_dev.button_dev.is_active = 1;
    DwinAllButtonsEnable(true);
}

void DwinGetCharHandler(uint8_t ch)
{
    switch(dwin_dev.state)
    {
        case DWIN_STATE_PRESTART:
            if (ch == 0x5A)
                dwin_dev.state = DWIN_STATE_START;
            else
                dwin_dev.state = DWIN_STATE_PRESTART;
            break;
        case DWIN_STATE_START:
            if (ch == 0xA5)
                dwin_dev.state = DWIN_STATE_LEN;
            else
                dwin_dev.state = DWIN_STATE_PRESTART;
            break;
        case DWIN_STATE_LEN:
            dwin_dev.data_len_cnt = 0;
            dwin_dev.data_len = ch;
            dwin_dev.state = DWIN_STATE_COMMAND;
            break;
        case DWIN_STATE_COMMAND:
            dwin_dev.cmd = ch;
            if (ch == 0x83)
            {
                dwin_dev.state = DWIN_STATE_READ_RAM_VPH;
                dwin_dev.vp_len_cnt = 0;
            }
            else
            {
                dwin_dev.state = DWIN_STATE_DATA;
            }

            dwin_dev.data_len_cnt++;
            break;
        case DWIN_STATE_READ_RAM_VPH:
            dwin_dev.vp_addr = ch << 8;
            dwin_dev.data_len_cnt++;
            dwin_dev.state = DWIN_STATE_READ_RAM_VPL;

            if (dwin_dev.data_len_cnt >= dwin_dev.data_len)
            {
                dwin_dev.state = DWIN_STATE_FINISH;
                break;
            }

            break;
        case DWIN_STATE_READ_RAM_VPL:
            dwin_dev.vp_addr += ch & 0xFF;
            dwin_dev.data_len_cnt++;
            dwin_dev.state = DWIN_STATE_READ_RAM_LEN;

            if (dwin_dev.data_len_cnt >= dwin_dev.data_len)
            {
                dwin_dev.state = DWIN_STATE_FINISH;
                break;
            }

            break;
        case DWIN_STATE_READ_RAM_LEN:
            dwin_dev.vp_len = ch;
            dwin_dev.data_len_cnt++;
            dwin_dev.state = DWIN_STATE_READ_RAM_DATA_H;

            if (dwin_dev.data_len_cnt >= dwin_dev.data_len)
            {
                dwin_dev.state = DWIN_STATE_FINISH;
                break;
            }

            break;
        case DWIN_STATE_READ_RAM_DATA_H:
            dwin_dev.vp_buf[dwin_dev.vp_len_cnt] = ch << 8;
            dwin_dev.data_len_cnt++;
            dwin_dev.state = DWIN_STATE_READ_RAM_DATA_L;

            if (dwin_dev.data_len_cnt >= dwin_dev.data_len)
            {
                dwin_dev.state = DWIN_STATE_FINISH;
                break;
            }

            break;
        case DWIN_STATE_READ_RAM_DATA_L:
            dwin_dev.vp_buf[dwin_dev.vp_len_cnt] += ch & 0xFF;
            dwin_dev.vp_len_cnt++;
            dwin_dev.data_len_cnt++;

            if(dwin_dev.vp_len_cnt >= dwin_dev.vp_len)
            {
                dwin_dev.state = DWIN_STATE_FINISH;
                break;
            }

            dwin_dev.state = DWIN_STATE_READ_RAM_DATA_H;
            if (dwin_dev.data_len_cnt >= dwin_dev.data_len)
            {
                dwin_dev.state = DWIN_STATE_FINISH;
                break;
            }
            break;
        case DWIN_STATE_DATA:
            dwin_dev.data[dwin_dev.data_len_cnt - 1] = ch;
            dwin_dev.data_len_cnt++;

            if (dwin_dev.data_len_cnt > dwin_dev.data_len)
            {
                dwin_dev.state = DWIN_STATE_FINISH;
                break;
            }

            break;
        case DWIN_STATE_FINISH:
            break;
        default:
            break;
    }

    if (dwin_dev.state == DWIN_STATE_FINISH)
    {
        
    }
}

bool DwinIsPushButton(uint16_t *button)
{
    if (button == NULL)
    {
        return false;
    }

    *button = 0;

    if (dwin_dev.state != DWIN_STATE_FINISH)
    {
        return false;
    }

    if (false == dwin_dev.button_dev.arr_active[dwin_dev.vp_addr & 0x00FF])
    {
        dwin_dev.state = DWIN_STATE_PRESTART;
        return false;
    }

    if (dwin_dev.button_dev.is_active && dwin_dev.vp_buf[0] == 0x0001)
    {
        dwin_dev.button_dev.is_active = false;
        *button = dwin_dev.vp_addr & 0xFFFF;
        return true;
    }

    dwin_dev.state = DWIN_STATE_PRESTART;
    return false;
}

void DwinHandleButton(uint16_t button)
{
    dwin_dev.state = DWIN_STATE_PRESTART;
    dwin_dev.button_dev.is_active = true;
}