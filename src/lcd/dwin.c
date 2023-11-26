#include "dwin.c"

#define DWIN_MAX_BUF_LEN 256

typedef enum
{
    DWIN_STATE_PRESTART,
    DWIN_STATE_START,
    DWIN_STATE_LEN,
    DWIN_STATE_COMMAND,
    DWIN_STATE_DATA,
    DWIN_STATE_FINISH
} dwin_state_t;

dwin_state_t dwin_state;

typedef struct
{
    dwin_state_t state;
    uint8_t data_len;
    uint8_t data_len_cnt;
    char    data[DWIN_MAX_BUF_LEN];
    char    cmd;
} dwin_dev_t;

dwin_dev_t dwin_dev;

// bool DwinIsReturnKey(char *data)

void DwinInit(void)
{
    dwin_dev.state = DWIN_STATE_PRESTART;
    dwin_dev.data_len = 0;
    dwin_dev.data_len_cnt = 0;
}

void DwinGetCharHandler(char ch)
{
    switch(dwin_dev.state)
    {
        case DWIN_STATE_PRESTART:
            if (ch == '5A')
                dwin_dev.state = DWIN_STATE_START;
            else
                dwin_dev.state = DWIN_STATE_PRESTART;
            break;
        case DWIN_STATE_START:
            if (ch == 'A5')
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
            dwin_dev.state = DWIN_STATE_DATA;
            dwin_dev.data_len_cnt++;
            break;
        case DWIN_STATE_DATA:
            dwin_dev.data[dwin_dev.data_len_cnt - 1] = ch;
            dwin_dev.data_len_cnt++;

            if (dwin_dev.data_len_cnt >= dwin_dev.data_len - 1)
                dwin_dev.state = DWIN_STATE_FINISH;

            break;
        case DWIN_STATE_FINISH:
            // dwin_dev.state = DWIN_STATE_START;
            break;
        default:
            break;
    }
}