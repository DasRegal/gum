#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "main.h"
#include "version.h"
#include "buf.h"
#include "mdb.h"
#include "coinbox.h"
#include "cashless.h"
#include "dwin.h"

extern void UsartDebugSendString(const char *pucBuffer);

#define BUF_MAX                 128
#define RCLI_ARGS_LENGTH        10
#define RCLI_ARGS_MAX_COUNT     10

static char buf[BUF_MAX];
static char rcli_out_buf[BUF_MAX];

static ctrlBuf_s bufStruct;
static char isSetColor;

#define RCLI_STR_WARNING(str)           (isSetColor ? "\e[38;5;11m" str "\e[0m" : str)
#define RCLI_STR_ERROR(str)             (isSetColor ? "\e[38;5;1m" str "\e[0m" : str)
#define RCLI_PROMPT_SET_COLOR_STR       "\r\e[38;5;15m\e[1mconsole>\e[0m "
#define RCLI_PROMPT_CLEAR_COLOR_STR     "\rconsole> "
#define RCLI_PROMPT(str)                (isSetColor ? "\e[38;5;15m\e[1m" str "\e[0m" : str)
#define RCLI_PROMPT_STR                 RCLI_PROMPT("\rConsole> ") 
#define RCLI_PROMPT_SHIFT 5    /* strlen of RCLI_PROMPT_STR */

typedef enum
{
    ESC_SEQ_OFF_STATE = 0,
    ESC_SEQ_ESC_SYM_STATE,      /* ESC symbol */
    ESC_SEQ_CSI_PREFIX_STATE,   /* ESC[ - Control Sequence Introducer (CSI) */
    ESC_SEQ_CSI_CUF_STATE,      /* ESC[ C - Cursor Forward */
    ESC_SEQ_CSI_CUB_STATE,      /* ESC[ D - Cursor Back */
    ESC_SEQ_LAST_STATE
} esc_seq_state_t;

typedef struct
{
    esc_seq_state_t cur_state;
    esc_seq_state_t matrix_state[ESC_SEQ_LAST_STATE][127];
} esc_seq_state_mach_t;

esc_seq_state_mach_t es_sm;

void init_esc_seq_state_machine(void)
{
    int i, j;
    for (i = 0; i < ESC_SEQ_LAST_STATE; i++)
        for (j = 0; j < 127; j++)
            es_sm.matrix_state[i][j] = ESC_SEQ_OFF_STATE;

    es_sm.cur_state = ESC_SEQ_OFF_STATE;

    es_sm.matrix_state[ESC_SEQ_OFF_STATE]['\e'] = ESC_SEQ_ESC_SYM_STATE;
    es_sm.matrix_state[ESC_SEQ_ESC_SYM_STATE]['['] = ESC_SEQ_CSI_PREFIX_STATE;
    es_sm.matrix_state[ESC_SEQ_CSI_PREFIX_STATE]['D'] = ESC_SEQ_CSI_CUB_STATE;
    es_sm.matrix_state[ESC_SEQ_CSI_PREFIX_STATE]['C'] = ESC_SEQ_CSI_CUF_STATE;
    es_sm.matrix_state[ESC_SEQ_CSI_CUF_STATE]['\e'] = ESC_SEQ_ESC_SYM_STATE;
    es_sm.matrix_state[ESC_SEQ_CSI_CUB_STATE]['\e'] = ESC_SEQ_ESC_SYM_STATE;
}

char RcliTransferChar(const char ch)
{
    char s[2];
    if (ch != '\0')
    {
        sprintf(s, "%c", ch);
        UsartDebugSendString(s);
    }

    return 0;
}

static char RcliTransferStr(char * const pBuf, unsigned char len)
{
    char *buf = pBuf;

    if (len > BUF_MAX)
    {
        len = BUF_MAX;
    }

    if (buf == NULL)
    {
        return 1;
    }
    
    while(len--)
    {
        if (RcliTransferChar(*buf++) != 0)
        {
            return 1;
        }
    }

    return 0;
}

void RcliInit(void)
{
    isSetColor = 1;
    buf_init(&bufStruct, buf, BUF_MAX);
    sprintf(rcli_out_buf, "\r\nWelcome (RCli %s)\n", RCLI_VERSION);
    RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
    
    sprintf(rcli_out_buf, "%s", RCLI_PROMPT_STR);
    RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));

    init_esc_seq_state_machine();
}

char echo_func_cmd(unsigned char args, void* argv)
{
    sprintf(rcli_out_buf, "Hello from echo callback!\r\n");
    RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));

    return 0;
}

char cmd_func_cmd(unsigned char args, void* argv)
{
    sprintf(rcli_out_buf, "Hello from cmd callback!%d - %s\r\n", args, (char *)(argv)+RCLI_ARGS_LENGTH*1);
    RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));

    return 0;
}

char status_func_cmd(unsigned char args, void* argv)
{
    sprintf(rcli_out_buf, "params=%d\r\n", args);
    RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));

    if (args == 2)
    {
        if (strcmp((char*)(argv) + RCLI_ARGS_LENGTH * 1, "get") == 0)
        {
            sprintf(rcli_out_buf, "[!] GET\r\n");
            RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
            return 0;
        }
    }

    if (args ==3)
    {
        if (strcmp((char*)(argv) + RCLI_ARGS_LENGTH * 1, "set") == 0)
        {
            int val = atoi((char*)(argv) + RCLI_ARGS_LENGTH * 2);
            sprintf(rcli_out_buf, "[!] SET %d\r\n", val);
            RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
            return 0;
        }
    }
    return -1;
}

char help_func_cmd(unsigned char args, void* argv);
char console_func_cmd(unsigned char args, void* argv);
char mdb_func_cmd(unsigned char args, void* argv);
char cctalk_func_cmd(unsigned char args, void* argv);
char cashless_func_cmd(unsigned char args, void* argv);
char dwin_func_cmd(unsigned char args, void* argv);

typedef char (*cb_t)(unsigned char args, void* argv);
typedef struct
{
    char argc;
    char ** argv;
    cb_t func;
} rcli_cmd_t;

rcli_cmd_t rcli_commands[] = 
{
    { 1, (char*[]){ "cashless", NULL }, cashless_func_cmd },
    { 1, (char*[]){ "cctalk", NULL }, cctalk_func_cmd },
    { 3, (char*[]){ "cmd", "ddd", "q", NULL }, cmd_func_cmd },
    { 1, (char*[]){ "console", NULL}, console_func_cmd },
    { 1, (char*[]){ "dwin", NULL}, dwin_func_cmd },
    { 1, (char*[]){ "echo", NULL}, echo_func_cmd },
    { 1, (char*[]){ "mdb", NULL}, mdb_func_cmd },
    { 3, (char*[]){ "status", "set", "get", NULL}, status_func_cmd },
    { 1, (char*[]){ "help", NULL}, help_func_cmd },
    { 1, (char*[]){ "?", NULL}, help_func_cmd }
};

#define RCLI_CMD_SIZE ( sizeof(rcli_commands) / sizeof(rcli_cmd_t) )

char help_func_cmd(unsigned char args, void* argv)
{
    for (int i = 0; i < RCLI_CMD_SIZE - 1; i++)
    {
        sprintf(rcli_out_buf, "  %s\r\n", (char*)rcli_commands[i].argv[0]);
        RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
    }
    return 0;
}

char dwin_func_cmd(unsigned char args, void* argv)
{
    char * help_str = 
"Usage: dwin <command>\r\n\r\n";
    char * help_str1 = 
"set page <n page>\t\tSet n page\r\n";

    char page = 0;

    if (args != 4)
    {
        sprintf(rcli_out_buf, "%s", help_str);
        RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
        sprintf(rcli_out_buf, "%s", help_str1);
        RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
        return 0;
    }

    page = (uint16_t)strtol((char*)(argv) + RCLI_ARGS_LENGTH * 3, NULL, 10);

    DwinSetPage(page);

    return 0;
}

char mdb_func_cmd(unsigned char args, void* argv)
{
    uint16_t buf[63];
    uint8_t  chk = 0;
    uint8_t  idx = 0;

    for(uint8_t i = 1; i < args; i++)
    {
        buf[i - 1] = (uint16_t)strtol((char*)(argv) + RCLI_ARGS_LENGTH * i, NULL, 16);
    }

    for( idx = 0; idx < args - 1; idx++)
    {
        chk += buf[idx] & 0xFF;
    }
    buf[idx] = chk;

    for(uint8_t i = 0; i < args; i++)
    {
        sprintf(rcli_out_buf, "0x%x ", buf[i]);
        RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
    }

    sprintf(rcli_out_buf, "\r\n");
    RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));

    //MdbBufSend(buf, args);
    return 0;
}

char console_func_cmd(unsigned char args, void* argv)
{
    char * help_str = 
"Usage: console [OPTION]\r\n\r\n\
  color set|clear\t\tSet color theme\r\n\r\n";

    if (args == 1)
    {
        sprintf(rcli_out_buf, "%s", help_str);
        RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
        return 0;
    }

    if (args == 2)
    {
        if (strcmp((char*)(argv) + RCLI_ARGS_LENGTH * 1, "color") == 0 ||
            strcmp((char*)(argv) + RCLI_ARGS_LENGTH * 1, "help") == 0 ||
            strcmp((char*)(argv) + RCLI_ARGS_LENGTH * 1, "?") == 0)
        {
            sprintf(rcli_out_buf, "%s", help_str);
            RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
            return 0;
        }
    }

    if (args == 3)
    {
        if (strcmp((char*)(argv) + RCLI_ARGS_LENGTH * 1, "color") == 0)
        {
            if (strcmp((char*)(argv) + RCLI_ARGS_LENGTH * 2, "set") == 0)
            {
                isSetColor = 1;
                sprintf(rcli_out_buf, "Example: %s %s %s\r\n", RCLI_STR_ERROR("Error"), RCLI_STR_WARNING("Warning"), RCLI_PROMPT("Prompt"));
                RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
                return 0;
            }
            if (strcmp((char*)(argv) + RCLI_ARGS_LENGTH * 2, "clear") == 0)
            {
                isSetColor = 0;
                sprintf(rcli_out_buf, "Example: %s %s %s\r\n", RCLI_STR_ERROR("Error"), RCLI_STR_WARNING("Warning"), RCLI_PROMPT("Prompt"));
                RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
                return 0;
            }
        }
    }

    sprintf(rcli_out_buf, "Bad params\r\n");
    RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
    return -1;
}

char cctalk_func_cmd(unsigned char args, void* argv)
{
    char * help_str = 
"Usage: cctalk <header> [<data_1>...<data_n>]\r\n\r\n";

    uint8_t hdr;
    uint8_t size;
    uint8_t buf[10];
    char *b;

    if (args == 1)
    {
        sprintf(rcli_out_buf, "%s", help_str);
        RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
        return 0;
    }

    hdr = (uint8_t)strtol((char*)(argv) + RCLI_ARGS_LENGTH * 1, NULL, 10);
    size = args - 2;

    for(uint8_t i = 0; i < size; i++)
    {
        buf[i] = (uint8_t)strtol((char*)(argv) + RCLI_ARGS_LENGTH * (i + 2), NULL, 10);
    }

    CoinBoxCliCmdSendData(hdr, buf, size);
    CoinBoxGetData(&b, &size);
    PRINT_OS("\r\n");
    for (uint8_t i = 0; i < size; i++)
    {
        sprintf(rcli_out_buf, "0x%x ", (uint8_t)b[i]);
        PRINT_OS(rcli_out_buf);
    }
    PRINT_OS("\r\n");
    for(uint8_t i = 0; i < size; i++)
    {
        if(b[i] >= 32 && b[i] < 127)
            sprintf(rcli_out_buf, "%c", b[i]);
        else
            sprintf(rcli_out_buf, " ");
        PRINT_OS(rcli_out_buf);
    }
    PRINT_OS("\r\n");
    return 0;
}

char cashless_func_cmd(unsigned char args, void* argv)
{
    char * help_str = 
"Usage: cashless <command>\r\n\r\n";
    char * help_str1 = 
"enable on|off\t\tForce enable On\\Off\r\n\
disable on|off\t\tForce disable On\\Off\r\n";
    char * help_str2 = 
"show state\t\tShow internal state of Cashless\r\n\
show info\t\tShow Information\r\n";
    char * help_str3 = 
"vend <price> <item>\tPush Vend request\r\n";
    char * help_str4 = 
"vend ok|fail\t\tIs dispensed\r\n";

    if (args == 1)
    {
        sprintf(rcli_out_buf, "%s", help_str);
        RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
        sprintf(rcli_out_buf, "%s", help_str1);
        RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
        sprintf(rcli_out_buf, "%s", help_str2);
        RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
        sprintf(rcli_out_buf, "%s", help_str3);
        RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
        sprintf(rcli_out_buf, "%s", help_str4);
        RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
        return 0;
    }

    if (args == 2)
    {
        if (strcmp((char*)(argv) + RCLI_ARGS_LENGTH * 1, "help") == 0 ||
            strcmp((char*)(argv) + RCLI_ARGS_LENGTH * 1, "enable") == 0 ||
            strcmp((char*)(argv) + RCLI_ARGS_LENGTH * 1, "disable") == 0 ||
            strcmp((char*)(argv) + RCLI_ARGS_LENGTH * 1, "?") == 0)
        {
            sprintf(rcli_out_buf, "%s", help_str);
            RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
            sprintf(rcli_out_buf, "%s", help_str1);
            RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
            sprintf(rcli_out_buf, "%s", help_str2);
            RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
            sprintf(rcli_out_buf, "%s", help_str3);
            RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
            return 0;
        }

        if (strcmp((char*)(argv) + RCLI_ARGS_LENGTH * 1, "reset") == 0)
        {
            CashlessResetCmd();
            sprintf(rcli_out_buf, "Cashless reset\r\n");
            RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
        }

        if (strcmp((char*)(argv) + RCLI_ARGS_LENGTH * 1, "vend") == 0)
        {
            sprintf(rcli_out_buf, "\r\n%s", help_str3);
            RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
            sprintf(rcli_out_buf, "\r\n%s", help_str4);
            RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
        }

        if (strcmp((char*)(argv) + RCLI_ARGS_LENGTH * 1, "show") == 0)
        {
            sprintf(rcli_out_buf, "\r\n%s\r\n", help_str2);
            RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
        }

        return 0;
    }

    if (args == 3)
    {
        if (strcmp((char*)(argv) + RCLI_ARGS_LENGTH * 1, "enable") == 0)
        {
            if (strcmp((char*)(argv) + RCLI_ARGS_LENGTH * 2, "on") == 0)
            {
                CashlessEnableForceCmd(true);
                RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
                return 0;
            }
            if (strcmp((char*)(argv) + RCLI_ARGS_LENGTH * 2, "off") == 0)
            {
                CashlessEnableForceCmd(false);
                RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
                return 0;
            }
        }

        if (strcmp((char*)(argv) + RCLI_ARGS_LENGTH * 1, "disable") == 0)
        {
            if (strcmp((char*)(argv) + RCLI_ARGS_LENGTH * 2, "on") == 0)
            {
                CashlessDisableForceCmd(true);
                RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
                return 0;
            }
            if (strcmp((char*)(argv) + RCLI_ARGS_LENGTH * 2, "off") == 0)
            {
                CashlessDisableForceCmd(false);
                RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
                return 0;
            }
        }

        if (strcmp((char*)(argv) + RCLI_ARGS_LENGTH * 1, "show") == 0)
        {
            if (strcmp((char*)(argv) + RCLI_ARGS_LENGTH * 2, "state") == 0)
            {
                uint8_t state1, state2, state3;
                char str[14];
                CashlessShowState(&state1, &state2, &state3);

                switch(state2)
                {
                    case MDB_STATE_INACTIVE:
                        sprintf(str, "Inactive");
                        break;
                    case MDB_STATE_DISABLED:
                        sprintf(str, "Disabled");
                        break;
                     case MDB_STATE_ENABLED:
                        sprintf(str, "Enabled");
                        break;
                     case MDB_STATE_SESSION_IDLE:
                        sprintf(str, "Session Idle");
                        break;
                     case MDB_STATE_VEND:
                        sprintf(str, "Vend");
                        break;
                     case MDB_STATE_REVALUE:
                        sprintf(str, "Revalue");
                        break;
                     case MDB_STATE_NEG_VEND:
                        sprintf(str, "Neg Vend");
                        break;
                    default:
                        sprintf(str, "Unknown");
                        break;
                }
                sprintf(rcli_out_buf, "Cashless State Machin: %s\r\nHelper state: %d\r\nHelper action: %d\r\n", str, state1, state3);
                RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
                return 0;
            }

            if (strcmp((char*)(argv) + RCLI_ARGS_LENGTH * 2, "info") == 0)
            {
                mdb_dev_t mdb_dev;

                mdb_dev = MdbGetDev();

                sprintf(rcli_out_buf, "Serial Number: %s\r\n", mdb_dev.dev_slave.serial_num);
                RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));

                sprintf(rcli_out_buf, "Model Number: %s\r\n", mdb_dev.dev_slave.model_num);
                RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));

                sprintf(rcli_out_buf, "Manufacture Code: %s\r\n", mdb_dev.dev_slave.manufact_code);
                RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));

                sprintf(rcli_out_buf, "SW Version: %d\r\n", mdb_dev.dev_slave.sw_version);
                RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));

                sprintf(rcli_out_buf, "Cashless MDB Level: %d. Master MDB Level: %d\r\n", mdb_dev.dev_slave.level, mdb_dev.level);
                RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));

                sprintf(rcli_out_buf, "Country Code: %d\r\n", mdb_dev.dev_slave.country_code);
                RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));

                sprintf(rcli_out_buf, "Scale Factor: %d\r\n", mdb_dev.dev_slave.scale_factor);
                RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));

                sprintf(rcli_out_buf, "Decimal Place: %d\r\n", mdb_dev.dev_slave.decimal_places);
                RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));

                sprintf(rcli_out_buf, "Max Resp Time: %d\r\n", mdb_dev.dev_slave.max_resp_time);
                RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));

                sprintf(rcli_out_buf, "Misc: %d\r\n", mdb_dev.dev_slave.misc);
                RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));

                return 0;
            }
        }

        if (strcmp((char*)(argv) + RCLI_ARGS_LENGTH * 1, "vend") == 0)
        {
            if (strcmp((char*)(argv) + RCLI_ARGS_LENGTH * 2, "ok") == 0)
            {
                if (CashlessVendSuccessCmd(0) != 0)
                    sprintf(rcli_out_buf, "Оплата не прошла.\r\n");
                else
                    sprintf(rcli_out_buf, "The selected product has been successfully dispensed.\r\n");

                RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
                return 0;
            }

            if (strcmp((char*)(argv) + RCLI_ARGS_LENGTH * 2, "fail") == 0)
            {
                CashlessVendFailureCmd();
                sprintf(rcli_out_buf, "The product was not dispensed.\r\n");
                RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
                return 0;
            }

            if (strcmp((char*)(argv) + RCLI_ARGS_LENGTH * 2, "cancel") == 0)
            {
                if (CashlessVendCancelCmd() != 0)
                    sprintf(rcli_out_buf, "Error. No Status Vend.\r\n");
                sprintf(rcli_out_buf, "Vend Cancel.\r\n");
                RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
                return 0;
            }

        }
    }

    if (args == 4)
    {
        if (strcmp((char*)(argv) + RCLI_ARGS_LENGTH * 1, "vend") == 0)
        {
            uint16_t price;
            uint16_t item;

            price = (uint16_t)strtol((char*)(argv) + RCLI_ARGS_LENGTH * 2, NULL, 10);
            item = (uint16_t)strtol((char*)(argv) + RCLI_ARGS_LENGTH * 3, NULL, 10);

            if (price == 0 || item == 0)
            {
                sprintf(rcli_out_buf, "Bad params\r\n");
                RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
                return -1;
            }

            sprintf(rcli_out_buf, "Price - %d, Item - %d\r\n", price, item);
            RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));

            if (CashlessVendRequest(price, item) != 0)
            {
                sprintf(rcli_out_buf, "Error. Not Session Idle\r\n");
                RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
            }

            return 0;
        }
    }

    sprintf(rcli_out_buf, "Bad params\r\n");
    RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
    return -1;
}

char rcli_parse_cmd(ctrlBuf_s bufStruct)
{
    char pos = -1;
    char res = -1;
    char *pBuf = NULL;
    
    res = buf_get_count_params(bufStruct); 
    
    if (res == -1)
    {
        return -1;
    }

    if (res == 0)
    {
        return 0;
    }

    /* Get first word position */
    pos = buf_get_pos_n_word(bufStruct, 0);

    if (pos == -1)
    {
        return -1;
    }

    int i = 0; 
    for (i = 0; i < RCLI_CMD_SIZE; i++)
    {
        if (rcli_commands[i].argv == NULL)
        {
            continue;
        }

        pBuf = buf_get_buf(bufStruct);
        char * pCmdPos = strstr(pBuf, rcli_commands[i].argv[0]);
        if (pCmdPos != pBuf + pos)
        {
            /* no callback function */
        }
        else
        {
            unsigned char args = 0; 
            char arr[RCLI_ARGS_MAX_COUNT][RCLI_ARGS_LENGTH];

            char * head = pBuf;
            do
            {
                while(*pBuf == 32)
                {
                    pBuf++;
                }
                head = pBuf;
                while(*pBuf != 32 && *pBuf != '\0')
                {
                    pBuf++;
                }
                strncpy(arr[args], head, pBuf - head);
                arr[args][pBuf-head] = '\0';
                if (head != pBuf)
                {
                    args++;
                }
            }
            while (*pBuf++);

            if (rcli_commands[i].func != NULL)
            {
                if (rcli_commands[i].func(args, (char**)arr) == -1)
                {
                    /* Return error */
                }
            }
            break;
        }
    }

    if (i >= RCLI_CMD_SIZE)
    {
        sprintf(rcli_out_buf, "Command not found\r\n");
        RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
        return -1;
    }

    return 0;
}

void rcli_parse_buf(char * buf)
{
    char *p_tmp;
    /* block uart */
    if (buf == NULL)
        return;
    
    if (buf[0]=='\0')
        return;

    p_tmp = buf;

    do
    {
        if (rcli_parse_cmd(bufStruct) == -1)
        {
        buf_debug(bufStruct);
            buf_clear(&bufStruct);
            break;
        }
        buf_debug(bufStruct);
        buf_clear(&bufStruct);
    }
    while(*p_tmp++);
    buf[0] = '\0';
    /* unblock uart */
}



void RcliUartHandler(uint8_t ch)
{
    int i = 0;
    int c;
    
    c = ch;

    if (c >= 0 && c <= 127)
        es_sm.cur_state = es_sm.matrix_state[es_sm.cur_state][c];

    switch(es_sm.cur_state)
    {
        case ESC_SEQ_OFF_STATE:
            break;
        case ESC_SEQ_CSI_CUB_STATE:
            if (bufStruct.cur_pos > bufStruct.end)
                return;
            buf_move_cur(&bufStruct, BUF_CUR_LEFT);
            PRINT_OS("\e[1D");
            return;
        case ESC_SEQ_CSI_CUF_STATE:
            if (bufStruct.cur_pos == 1)
                return;
            buf_move_cur(&bufStruct, BUF_CUR_RIGHT);
            PRINT_OS("\e[1C");
            return;
        default:
            return;
    }

    /* BACKSPACE */
    if (c == 127 || c == 8)
    {
        if (bufStruct.end + 1 - bufStruct.cur_pos == 0)
            return;
        buf_move_cur(&bufStruct, BUF_CUR_LEFT);
        buf_del(&bufStruct);
        PRINT_OS("\e[1D");

        sprintf(rcli_out_buf, "%s ", buf_get_buf_from_pos(bufStruct));
        RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));

        sprintf(rcli_out_buf, "\e[%dD", bufStruct.cur_pos);
        RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
        return;
    }

    /* LETTER */ 
    if ((c >= 32 && c < 127))
    {
        buf_add(&bufStruct, c, 0);

        if(bufStruct.cur_pos != 1)
        {
            char pos = bufStruct.end - bufStruct.cur_pos;
            if (pos > 0)
            {
                sprintf(rcli_out_buf, "\e[%dD", pos);
                RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
            }
            sprintf(rcli_out_buf, "%s", bufStruct.pBuf);
            RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
            sprintf(rcli_out_buf, "\e[%dD", bufStruct.cur_pos - 1);
            RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
        }
        else
            RcliTransferChar(c);

        if (i >= BUF_MAX)
            return;
        i++;
    }

    /* ENTER */
    if (c == 13)
    {
        sprintf(rcli_out_buf, "\r\n");
        RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
        rcli_parse_buf(bufStruct.pBuf);

        buf_clear(&bufStruct);

        sprintf(rcli_out_buf, "%s", RCLI_PROMPT_STR);
        RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
    }

    /* TAB */
    if (c == 9)
    {
        if( buf_get_count_params(bufStruct) == 1)
        {
            sprintf(rcli_out_buf, "\r\n");
            RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
           
            int i;
            int count_words = 0;
            int k = 0;
            char * cmd_str;
            for (i = 0; i < RCLI_CMD_SIZE - 1; i++)
            {
                cmd_str = (char*)rcli_commands[i].argv[0];

                char * pstr = strstr(cmd_str, bufStruct.pBuf);
                if(pstr == rcli_commands[i].argv[0])
                {
                    sprintf(rcli_out_buf, "  %s\r\n", cmd_str);
                    RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
                    k = i;
                    count_words++;
                }
            }
            sprintf(rcli_out_buf, "%s", RCLI_PROMPT_STR);
            RcliTransferStr(rcli_out_buf, strlen(rcli_out_buf));
            if(count_words == 1)
            {
                cmd_str = (char*)rcli_commands[k].argv[0]; 
                buf_cpy_str(&bufStruct, cmd_str, strlen(cmd_str));
                buf_add(&bufStruct, ' ', 0);
                RcliTransferStr(bufStruct.pBuf, bufStruct.end);
            }
            else
            {
                RcliTransferStr(bufStruct.pBuf, strlen(bufStruct.pBuf));
            }
        }
    }
}
