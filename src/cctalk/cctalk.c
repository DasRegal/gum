#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "cctalk.h"

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
static char CctalkSimpleCheckSum(cctalk_data_t data);

int CctalkInit(cctalk_master_dev_t dev)
{
    if (dev.send_data_cb == NULL)
    {
        return 1;
    }
    cctalk_dev = dev;

    return 0;
}

void CctalkSendData(cctalk_data_t data)
{
    data.buf[0] = data.dest_addr;
    data.buf[1] = data.buf_len;
    data.buf[2] = data.src_addr;
    data.buf[3] = data.header;
    data.buf[data.buf_len + 4] = CctalkSimpleCheckSum(data);

    cctalk_dev.send_data_cb(data.buf);
}

static char CctalkSimpleCheckSum(cctalk_data_t data)
{
    uint16_t        i;
    uint8_t         sum = 0;
    for (i = 0; i < data.buf_len + 4; i++)
    {
        sum = (sum + data.buf[i]) % 256;
    }
    sum = 256 - sum;

    return sum;
}

void CctalkUartHandler(uint8_t ch)
{
    char buf[2];
    sprintf(buf, "%c", (char)ch);
    UsartDebugSendString(buf);
}