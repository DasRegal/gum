#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "dwin.h"

dwin_dev_t dwin_dev;

static void DwinBufSend(char * buf, uint8_t len);
static void DwinWriteCmd(uint16_t vp, char *data, uint8_t len);

void DwinHandleButton(uint16_t button)
{
    dwin_dev.state = DWIN_STATE_PRESTART;
    dwin_dev.button_dev.is_active = true;
}

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

void DwinInit(void (*cb_send)(const char*, uint8_t))
{
    dwin_dev.state = DWIN_STATE_PRESTART;
    dwin_dev.data_len = 0;
    dwin_dev.data_len_cnt = 0;
    dwin_dev.vp_len = 0;
    dwin_dev.vp_len_cnt = 0;
    dwin_dev.button_dev.is_pushed = 0;
    dwin_dev.button_dev.is_active = 1;
    if (cb_send != NULL)
        dwin_dev.cb_send = cb_send;
    DwinAllButtonsEnable(true);
}

bool DwinGetCharHandler(uint8_t ch)
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
            if (ch == 0x83 || ch == 0x82)
            {
                dwin_dev.state = DWIN_STATE_READ_RAM_VPH;
                dwin_dev.vp_len_cnt = 0;
            }
            else
            {
                dwin_dev.state = DWIN_STATE_FINISH;
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

            if ( dwin_dev.cmd == 0x83)
                dwin_dev.state = DWIN_STATE_READ_RAM_LEN;
            else
                dwin_dev.state = DWIN_STATE_RW_RAM_DATA_H;

            if (dwin_dev.data_len_cnt >= dwin_dev.data_len)
            {
                dwin_dev.state = DWIN_STATE_FINISH;
                break;
            }

            break;
        case DWIN_STATE_READ_RAM_LEN:
            dwin_dev.vp_len = ch;
            dwin_dev.data_len_cnt++;
            dwin_dev.state = DWIN_STATE_RW_RAM_DATA_H;

            if (dwin_dev.data_len_cnt >= dwin_dev.data_len)
            {
                dwin_dev.state = DWIN_STATE_FINISH;
                break;
            }

            break;
        case DWIN_STATE_RW_RAM_DATA_H:
            dwin_dev.vp_buf[dwin_dev.vp_len_cnt] = ch << 8;
            dwin_dev.data_len_cnt++;
            dwin_dev.state = DWIN_STATE_RW_RAM_DATA_L;

            if (dwin_dev.data_len_cnt >= dwin_dev.data_len)
            {
                dwin_dev.state = DWIN_STATE_FINISH;
                break;
            }

            break;
        case DWIN_STATE_RW_RAM_DATA_L:
            dwin_dev.vp_buf[dwin_dev.vp_len_cnt] += ch & 0xFF;
            dwin_dev.vp_len_cnt++;
            dwin_dev.data_len_cnt++;

            if(dwin_dev.vp_len_cnt >= dwin_dev.vp_len)
            {
                dwin_dev.state = DWIN_STATE_FINISH;
                break;
            }

            dwin_dev.state = DWIN_STATE_RW_RAM_DATA_H;
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
        if (dwin_dev.cmd == 0x82)
            DwinHandleButton(0);

        return true;
    }

    return false;
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

void DwinSetPage(uint8_t page)
{
    char buf[4] = { 0x5A, 0x01 , 0x00, 0x00 };

    buf[3] = page;
    DwinWriteCmd(0x0084, buf, 4);
}

void DwinReset(void)
{
    char buf[4] = { 0x55, 0xAA , 0x5A, 0xA5 };
    DwinWriteCmd(0x0004, buf, 4);
}

void DwinButtonEn(uint16_t button, bool isEnable)
{
    char buf[2] = { 0, 0 };

    if (dwin_dev.button_dev.arr_active[button & 0x00FF] == isEnable)
        return;

    dwin_dev.button_dev.arr_active[button & 0x00FF] = isEnable;

    if(!isEnable)
    {
        buf[1] = 1;
    }

    DwinWriteCmd(button, buf, 2);   
}

dwin_dev_t *DwinGetDev(void)
{
    return &dwin_dev;
}

static void DwinBufSend(char * buf, uint8_t len)
{
    if (dwin_dev.cb_send != NULL)
        dwin_dev.cb_send(buf, len);
}

static void DwinWriteCmd(uint16_t vp, char *data, uint8_t len)
{
    char buf[6] = { 0x5A, 0xA5, 0x00, 0x82, 0x00, 0x00 };

    if (len > 0xFF - 1)
        return;

    buf[2] = len + 3;
    buf[4] = (char)((vp >> 8) & 0xFF);
    buf[5] = (char)(vp & 0xFF);
    DwinBufSend(buf, 6);

    DwinBufSend(data, len);
}
