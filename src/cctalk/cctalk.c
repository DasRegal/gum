#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "cctalk.h"
#include "main.h"

typedef struct
{
    char short_name[4];
    char full_name[45];
} cctalk_manufacturer_id_t;

cctalk_manufacturer_id_t cctalk_manufacture_id[17] = 
{
    { "AES", "Aardvark Embedded Solutions Ltd"            },
    { "ALB", "Alberici"                                   },
    { "AST", "AstroSystems Ltd"                           },
    { "AZK", "Azkoyen"                                    },
    { "ECP", "Encopim SL"                                 },
    { "CMG", "Comestero Group"                            },
    { "GTD", "Gaming Technology Distribution"             },
    { "HIM", "Himecs"                                     },
    { "ITL", "Innovative Technology Ltd"                  },
    { "INT", "Intergrated(sic) Technology Ltd"            },
    { "ICT", "International Currency Technologies"        },
    { "JCM", "Japan Cash Machine"                         },
    { "MEI", "Mars Electronics International"             },
    { "MSC", "Microsystem Controls Pty. Ltd. (Microcoin)" },
    { "MCI", "Money Controls (International)"             },
    { "NRI", "National Rejectors Inc"                     },
    { "SEL", "Starpoint Electrics Ltd"                    },
};

typedef struct
{
    char cat_str[20];
    uint8_t addr;
} cctalk_equip_cat_id_t;

cctalk_equip_cat_id_t cctalk_equip_cat_id[12] = 
{
    { "Coin Acceptor",  2   },
    { "Payout",         3   },
    { "Reel",           30  },
    { "Bill Validator", 40  },
    { "Card Reader",    50  },
    { "Display",        60  },
    { "Keypad",         70  },
    { "Dongle",         80  },
    { "Meter",          90  },
    { "Power",          100 },
    { "Printer",        110 },
    { "RNG",            120 },
};

char *cctalk_self_check_err_code[44] = 
{
    "OK",
    "EEPROM checksum corrupted",
    "Fault on inductive coils",
    "Fault on credit sensor",
    "Fault on piezo sensor",
    "Fault on reflective sensor ",
    "Fault on diameter sensor",
    "Fault on wake-up sensor",
    "Fault on sorter exit sensors",
    "NVRAM checksum corrupted",
    "Coin dispensing error",
    "Low level sensor error",
    "High level sensor error",
    "Coin counting error",
    "Keypad error",
    "Button error",
    "Display error",
    "Coin auditing error",
    "Fault on reject sensor",
    "Fault on coin return mechanism",
    "Fault on C.O.S. mechanism",
    "Fault on rim sensor",
    "Fault on thermistor",
    "Payout motor fault",
    "Payout timeout",
    "Payout jammed",
    "Payout sensor fault",
    "Level sensor error",
    "Personality module not fitted",
    "Personality checksum corrupted -",
    "ROM checksum mismatch",
    "Missing slave device",
    "Internal comms bad",
    "Supply voltage outside operating limits",
    "Temperature outside operating limits",
    "D.C.E. fault",
    "Fault on bill validation sensor",
    "Fault on bill transport motor",
    "Fault on stacker ",
    "Bill jammed",
    "RAM test fail",
    "Fault on string sensor",
    "Accept gate failed open",
    "Accept gate failed closed"
};

// static char                 cctalk_buf[CCTALK_MAX_BUF_LEN];
// static cctalk_data_t        cctalk_data = { .buf          = cctalk_buf };
static cctalk_master_dev_t  cctalk_dev  = { .send_data_cb = NULL,
                                            .master_addr  = 1,
                                            .slave_addr   = 2 };

extern void UsartDebugSendString(const char *pucBuffer);
static char CctalkSimpleCheckSum(void);
static void CctalkUpdateBalance(void);
// static char CctalkSimpleCheckSum(cctalk_data_t data);

int CctalkInit(cctalk_master_dev_t dev)
{
    if (dev.send_data_cb == NULL)
    {
        return 1;
    }
    // cctalk_dev = dev;
    cctalk_dev.slave_addr = 2;
    cctalk_dev.master_addr = 1;
    cctalk_dev.buf_cnt = 0;
    cctalk_dev.buf_data_len = 0;
    cctalk_dev.send_data_cb = dev.send_data_cb;
    cctalk_dev.state = CCTALK_STATE_DST_ADDR;
    cctalk_dev.credit.event = 0;
    cctalk_dev.credit.balance = 0;
    cctalk_dev.credit.is_first_event = true;

    return 0;
}

void CctalkSendData(uint8_t hdr, uint8_t *data, uint8_t size)
{
    cctalk_dev.buf_tx[0] = cctalk_dev.slave_addr;
    cctalk_dev.buf_tx[1] = size;
    cctalk_dev.buf_tx[2] = cctalk_dev.master_addr;
    cctalk_dev.buf_tx[3] = hdr;

    if (data != NULL)
    {
        for(uint8_t i = 0; i < size; i++)
        {
            cctalk_dev.buf_tx[i + 4] = data[i];
        }
    }

    cctalk_dev.buf_tx[size + 4] = CctalkSimpleCheckSum();
    cctalk_dev.send_data_cb(cctalk_dev.buf_tx);
    CctalkAnswerHandle();
}

// void CctalkSendData(cctalk_data_t data)
// {
//     data.buf[0] = data.dest_addr;
//     data.buf[1] = data.buf_len;
//     data.buf[2] = data.src_addr;
//     data.buf[3] = data.header;
//     data.buf[data.buf_len + 4] = CctalkSimpleCheckSum(data);

//     // cctalk_dev.buf_cnt = 0;
//     // cctalk_dev.buf_data_len = 0;

//     cctalk_dev.send_data_cb(data.buf);
//     CctalkAnswerHandle();
// }

static char CctalkSimpleCheckSum(void)
{
    uint16_t        i;
    uint8_t         sum = 0;
    for (i = 0; i < cctalk_dev.buf_tx[1] + 4; i++)
    {
        sum = (sum + cctalk_dev.buf_tx[i]) % 256;
    }
    sum = 256 - sum;

    return sum;
}

// static char CctalkSimpleCheckSum(cctalk_data_t data)
// {
//     uint16_t        i;
//     uint8_t         sum = 0;
//     for (i = 0; i < data.buf_len + 4; i++)
//     {
//         sum = (sum + data.buf[i]) % 256;
//     }
//     sum = 256 - sum;

//     return sum;
// }

bool CctalkGetCharHandler(uint8_t ch)
{
    switch(cctalk_dev.state)
    {
        case CCTALK_STATE_DST_ADDR:
            cctalk_dev.buf_cnt = 0;
            cctalk_dev.crc_sum = 0;
            cctalk_dev.crc_sum = (cctalk_dev.crc_sum + (uint8_t)ch) % 256;

            cctalk_dev.state = CCTALK_STATE_LEN;
            break;
        case CCTALK_STATE_LEN:
            cctalk_dev.buf_data_len = ch;
            cctalk_dev.crc_sum = (cctalk_dev.crc_sum + (uint8_t)ch) % 256;
            cctalk_dev.state = CCTALK_STATE_SRC_ADDR;
            break;
        case CCTALK_STATE_SRC_ADDR:
            cctalk_dev.crc_sum = (cctalk_dev.crc_sum + (uint8_t)ch) % 256;
            cctalk_dev.state = CCTALK_STATE_HDR;
            break;
        case CCTALK_STATE_HDR:
            cctalk_dev.crc_sum = (cctalk_dev.crc_sum + (uint8_t)ch) % 256;

            if(cctalk_dev.buf_data_len == 0)
            {
                cctalk_dev.state = CCTALK_STATE_CRC;
            }
            else
            {
                cctalk_dev.state = CCTALK_STATE_DATA;
            }
            break;
        case CCTALK_STATE_DATA:
            cctalk_dev.buf[cctalk_dev.buf_cnt] = ch;
            cctalk_dev.crc_sum = (cctalk_dev.crc_sum + (uint8_t)ch) % 256;
            cctalk_dev.buf_cnt++;

            if(cctalk_dev.buf_cnt >= cctalk_dev.buf_data_len)
            {
                cctalk_dev.state = CCTALK_STATE_CRC;
                break;
            }
            break;
        case CCTALK_STATE_CRC:
            cctalk_dev.crc_sum = (cctalk_dev.crc_sum + (uint8_t)ch) % 256;
            cctalk_dev.crc_sum = 256 - cctalk_dev.crc_sum;
            if (cctalk_dev.crc_sum == 0)
            {
                cctalk_dev.state = CCTALK_STATE_FINISH;
                // cctalk_dev.state = CCTALK_STATE_DST_ADDR;
                // return true;
            }
            else
            {
                cctalk_dev.state = CCTALK_STATE_DST_ADDR;
            }
            break;
        case CCTALK_STATE_FINISH:
            break;
        default: 
            cctalk_dev.state = CCTALK_STATE_DST_ADDR;
            break;
    }

    if (cctalk_dev.state == CCTALK_STATE_FINISH)
    {
        uint8_t hdr = cctalk_dev.buf_tx[3];


        switch(hdr)
        {
            case CCTALK_HDR_READ_BUF_CREDIT:
                if (cctalk_dev.credit.is_first_event)
                {
                    cctalk_dev.credit.is_first_event = false;
                    cctalk_dev.credit.event = cctalk_dev.buf[0];
                    break;
                }

                if (cctalk_dev.credit.event == cctalk_dev.buf[0])
                {
                    break;
                }

                CctalkUpdateBalance();
                break;
            case CCTALK_HDR_REQ_COIN_ID:
                break;
            default:
                break;
        }

        return true;
    }

    return false;
}

static void CctalkUpdateBalance(void)
{
    uint8_t event;
    uint8_t last_event;
    uint8_t res;
    uint8_t idx;
    uint8_t mult;

    struct
    {
        uint8_t coin;
        uint8_t sorter;
    } cr[5];

    event       = cctalk_dev.buf[0];
    last_event  = cctalk_dev.credit.event;

    res = event - last_event;
    if (event < last_event) res--;
    cctalk_dev.credit.event = event;

    if (res > 5) res = 5;

    for (uint8_t i = 0; i < res; i++)
    {
        idx = (i * 2) + 1;
        cr[i].coin   = cctalk_dev.buf[idx];
        cr[i].sorter = cctalk_dev.buf[idx + 1];
    }

    switch(cr[0].coin)
    {
        case 1:
        case 2:
            mult = 1;
            break;
        case 3:
        case 4:
            mult = 2;
            break;
        case 5:
        case 6:
            mult = 5;
            break;
        case 7:
        case 8:
            mult = 10;
            break;
        default:
            mult = 1;
    }

    cctalk_dev.credit.balance += 1 * mult;
}

cctalk_master_dev_t *CctalkGetDev(void)
{
    return &cctalk_dev;
}

void CctalkAnswerHandle(void)
{
    cctalk_dev.state = CCTALK_STATE_DST_ADDR;
}

bool CctalkIsEnable(void)
{
    return true;
}

void CctalkTestFunc(void)
{
    // char buf[10];
    // cctalk_data_t data = { .buf = buf };
    // data.dest_addr = 2;
    // data.buf_len = 0;
    // data.src_addr = 1;
    // data.header = 242;
    // CctalkSendData(data);

    CctalkSendData(CCTALK_HDR_REQ_SERIAL_NUM, NULL, 0);
}