#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "mdb.h"
// #define TEST_MDB

extern void MdbSendCommand(uint8_t cmd, uint8_t subcmd, uint8_t * data);
extern void MdbSetLevel(mdb_level_t level);
extern void MdbSetExpansion(bool is_exp);
static void TestBufPrint(const uint16_t *pucBuffer, uint8_t len);

void main(void)
{
    uint8_t buf[36];
    mdv_dev_init_struct_t mdb_dev_struct;
    mdb_dev_struct.send_callback = TestBufPrint;
    MdbInit(mdb_dev_struct);

    printf("RESET CMD (0x110 0x10)\n");
    printf("           ");
    MdbSendCommand(MDB_RESET_CMD_E, 0, NULL);
    printf("\n");

    printf("SETUP CMD (0x111 0x0 0x3 0x1 0x2 0x3 0x1a)\n");
    printf("           ");
    buf[0] = 0x03;
    buf[1] = 1;
    buf[2] = 2;
    buf[3] = 3;
    MdbSendCommand(MDB_SETUP_CMD_E, MDB_SETUP_CONF_DATA_SUBCMD, buf);
    printf("\n");

    printf("SETUP CMD (0x111 0x1 0x0 0x64 0x0 0x32 0xa8)\n");
    printf("           ");
    buf[0] = 0;
    buf[1] = 0x64;   /* Max price */
    buf[2] = 0;
    buf[3] = 0x32;     /* Min price */
    MdbSendCommand(MDB_SETUP_CMD_E, MDB_SETUP_PRICE_SUBCMD, buf);
    printf("\n");

    printf("POLL CMD (0x112 0x12)\n");
    printf("          ");
    MdbSendCommand(MDB_POLL_CMD_E, 0, NULL);
    printf("\n");

    printf("VEND CMD (0x113 0x0 0x0 0x1 0x2 0x3 0x19)\n");
    printf("          ");
    buf[0] = 0;
    buf[1] = 1;
    buf[2] = 2;
    buf[3] = 3;
    MdbSendCommand(MDB_VEND_CMD_E, MDB_VEND_REQ_SUBCMD, buf);
    printf("\n");

    MdbSetLevel(MDB_LEVEL_3);
    printf("VEND CMD (0x113 0x0 0x0 0x1 0x2 0x3 0x19) L3 \n");
    printf("          ");
    buf[0] = 0;
    buf[1] = 1;
    buf[2] = 2;
    buf[3] = 3;
    buf[4] = 4;
    buf[5] = 5;
    MdbSendCommand(MDB_VEND_CMD_E, MDB_VEND_REQ_SUBCMD, buf);
    printf("\n");
    MdbSetLevel(MDB_LEVEL_2);
    MdbSetExpansion(false);

    MdbSetLevel(MDB_LEVEL_3);
    MdbSetExpansion(true);
    printf("VEND CMD (0x113 0x0 0x0 0x1 0x2 0x3 0x4 0x5 0x22) L3+EXP \n");
    printf("          ");
    buf[0] = 0;
    buf[1] = 1;
    buf[2] = 2;
    buf[3] = 3;
    buf[4] = 4;
    buf[5] = 5;
    MdbSendCommand(MDB_VEND_CMD_E, MDB_VEND_REQ_SUBCMD, buf);
    printf("\n");
    MdbSetLevel(MDB_LEVEL_2);
    MdbSetExpansion(false);

    printf("VEND CMD (0x113 0x1 0x14)\n");
    printf("          ");
    MdbSendCommand(MDB_VEND_CMD_E, MDB_VEND_CANCEL_SUBCMD, buf);
    printf("\n");

    printf("VEND CMD (0x113 0x2 0x0 0x1 0x16) \n");
    printf("          ");
    buf[0] = 0;
    buf[1] = 1;
    MdbSendCommand(MDB_VEND_CMD_E, MDB_VEND_SUCCESS_SUBCMD, buf);
    printf("\n");

    printf("VEND CMD (0x113 0x3 0x16)\n");
    printf("          ");
    MdbSendCommand(MDB_VEND_CMD_E, MDB_VEND_FAILURE_SUBCMD, buf);
    printf("\n");

    printf("VEND CMD (0x113 0x4 0x17)\n");
    printf("          ");
    MdbSendCommand(MDB_VEND_CMD_E, MDB_VEND_SESS_COMPL_SUBCMD, buf);
    printf("\n");

    printf("VEND CMD (0x113 0x5 0x0 0x1 0x2 0x3 0x1e)\n");
    printf("          ");
    buf[0] = 0;
    buf[1] = 1;
    buf[2] = 2;
    buf[3] = 3;
    MdbSendCommand(MDB_VEND_CMD_E, MDB_VEND_CASH_SALE_SUBCMD, buf);
    printf("\n");

    MdbSetLevel(MDB_LEVEL_3);
    printf("VEND CMD (0x113 0x5 0x0 0x1 0x2 0x3 0x1e) L3\n");
    printf("          ");
    buf[0] = 0;
    buf[1] = 1;
    buf[2] = 2;
    buf[3] = 3;
    MdbSendCommand(MDB_VEND_CMD_E, MDB_VEND_CASH_SALE_SUBCMD, buf);
    printf("\n");
    MdbSetLevel(MDB_LEVEL_2);
    MdbSetExpansion(false);

    MdbSetLevel(MDB_LEVEL_3);
    MdbSetExpansion(true);
    printf("VEND CMD (0x113 0x5 0x0 0x1 0x2 0x3 0x4 0x5 0x6 0x7 0x34) L3+EXP\n");
    printf("          ");
    buf[0] = 0;
    buf[1] = 1;
    buf[2] = 2;
    buf[3] = 3;
    buf[4] = 4;
    buf[5] = 5;
    buf[6] = 6;
    buf[7] = 7;
    MdbSendCommand(MDB_VEND_CMD_E, MDB_VEND_CASH_SALE_SUBCMD, buf);
    printf("\n");
    MdbSetLevel(MDB_LEVEL_2);
    MdbSetExpansion(false);

    printf("VEND CMD (0x113 0x6...) not supported for L1, L2\n");
    printf("          ");
    MdbSendCommand(MDB_VEND_CMD_E, MDB_VEND_NEG_REQ_SUBCMD, buf);
    printf("\n");

    MdbSetLevel(MDB_LEVEL_3);
    MdbSetExpansion(false);
    printf("VEND CMD (0x113 0x6 0x0 0x1 0x2 0x3 0x1f) L3\n");
    printf("          ");
    buf[0] = 0;
    buf[1] = 1;
    buf[2] = 2;
    buf[3] = 3;
    MdbSendCommand(MDB_VEND_CMD_E, MDB_VEND_NEG_REQ_SUBCMD, buf);
    printf("\n");
    MdbSetLevel(MDB_LEVEL_2);
    MdbSetExpansion(false);

    MdbSetLevel(MDB_LEVEL_3);
    MdbSetExpansion(true);
    printf("VEND CMD (0x113 0x6 0x0 0x1 0x2 0x3 0x4 0x5 0x28) L3+EXP\n");
    printf("          ");
    buf[0] = 0;
    buf[1] = 1;
    buf[2] = 2;
    buf[3] = 3;
    buf[4] = 4;
    buf[5] = 5;
    MdbSendCommand(MDB_VEND_CMD_E, MDB_VEND_NEG_REQ_SUBCMD, buf);
    printf("\n");
    MdbSetLevel(MDB_LEVEL_2);
    MdbSetExpansion(false);

    printf("READER CMD (0x114 0x0 0x14)\n");
    printf("            ");
    MdbSendCommand(MDB_READER_CMD_E, MDB_READER_DISABLE_SUBCMD, buf);
    printf("\n");

    printf("READER CMD (0x114 0x1 0x15)\n");
    printf("            ");
    MdbSendCommand(MDB_READER_CMD_E, MDB_READER_ENABLE_SUBCMD, buf);
    printf("\n");

    printf("READER CMD (0x114 0x2 0x16)\n");
    printf("            ");
    MdbSendCommand(MDB_READER_CMD_E, MDB_READER_CANCEL_SUBCMD, buf);
    printf("\n");

    MdbSetLevel(MDB_LEVEL_1);
    MdbSetExpansion(false);
    printf("REVALUE CMD (0x115 0x0...) not supported for L1\n");
    printf("             ");
    MdbSendCommand(MDB_REVALUE_CMD_E, MDB_REVALUE_REQ_SUBCMD, buf);
    printf("\n");
    MdbSetLevel(MDB_LEVEL_2);
    MdbSetExpansion(false);

    MdbSetLevel(MDB_LEVEL_3);
    MdbSetExpansion(false);
    printf("REVALUE CMD (0x115 0x0 0x1 0x2 0x18)\n");
    printf("             ");
    buf[0] = 1;
    buf[1] = 2;
    MdbSendCommand(MDB_REVALUE_CMD_E, MDB_REVALUE_REQ_SUBCMD, buf);
    printf("\n");
    MdbSetLevel(MDB_LEVEL_2);
    MdbSetExpansion(false);

    MdbSetLevel(MDB_LEVEL_1);
    MdbSetExpansion(true);
    printf("REVALUE CMD (0x115 0x1...) not supported for L1\n");
    printf("             ");
    MdbSendCommand(MDB_REVALUE_CMD_E, MDB_REVALUE_LIM_REQ_SUBCMD, buf);
    printf("\n");
    MdbSetLevel(MDB_LEVEL_2);
    MdbSetExpansion(false);

    MdbSetLevel(MDB_LEVEL_3);
    MdbSetExpansion(false);
    printf("REVALUE CMD (0x115 0x1 0x16)\n");
    printf("             ");
    MdbSendCommand(MDB_REVALUE_CMD_E, MDB_REVALUE_LIM_REQ_SUBCMD, buf);
    printf("\n");
    MdbSetLevel(MDB_LEVEL_2);
    MdbSetExpansion(false);

    MdbSetLevel(MDB_LEVEL_2);
    MdbSetExpansion(false);
    printf("EXP CMD (0x117 0x0 0x1 0x2 0x3 0x0 ... 0x0 0x3 0x2 0x1 0x23)\n");
    printf("         ");
    memset(buf, 0, 29);
    buf[0] = 1;
    buf[1] = 2;
    buf[2] = 3;
    buf[28] = 1;
    buf[27] = 2;
    buf[26] = 3;
    MdbSendCommand(MDB_EXPANSION_CMD_E, MDB_EXP_REQ_ID_SUBCMD, buf);
    printf("\n");
    MdbSetLevel(MDB_LEVEL_2);
    MdbSetExpansion(false);

    MdbSetLevel(MDB_LEVEL_2);
    MdbSetExpansion(false);
    printf("EXP CMD (0x117 0x1...) L2 Depricated\n");
    printf("         ");
    buf[0] = 1;
    MdbSendCommand(MDB_EXPANSION_CMD_E, MDB_EXP_READ_FILE_SUBCMD, buf);
    printf("\n");

    MdbSetLevel(MDB_LEVEL_2);
    MdbSetExpansion(false);
    printf("EXP CMD (0x117 0x2...) L2 Depricated\n");
    printf("         ");
    buf[0] = 1;
    MdbSendCommand(MDB_EXPANSION_CMD_E, MDB_EXP_WRITE_FILE_SUBCMD, buf);
    printf("\n");

    MdbSetLevel(MDB_LEVEL_2);
    MdbSetExpansion(false);
    printf("EXP CMD (0x117 0x3...) Just not supported\n");
    printf("         ");
    MdbSendCommand(MDB_EXPANSION_CMD_E, MDB_EXP_W_TIME_DATA_SUBCMD, buf);
    printf("\n");

    MdbSetLevel(MDB_LEVEL_2);
    MdbSetExpansion(false);
    printf("EXP CMD (0x117 0x4 0x1 0x2 0x3 0x4 0x23) not supported for L1, L2\n");
    printf("         ");
    buf[0] = 1;
    buf[1] = 2;
    buf[2] = 3;
    buf[3] = 4;
    MdbSendCommand(MDB_EXPANSION_CMD_E, MDB_EXP_OPT_FTR_EN_SUBCMD, buf);
    printf("\n");
    MdbSetLevel(MDB_LEVEL_2);
    MdbSetExpansion(false);

    MdbSetLevel(MDB_LEVEL_3);
    MdbSetExpansion(false);
    printf("EXP CMD (0x117 0x4 0x1 0x2 0x3 0x4 0x25) L3\n");
    printf("         ");
    buf[0] = 1;
    buf[1] = 2;
    buf[2] = 3;
    buf[3] = 4;
    MdbSendCommand(MDB_EXPANSION_CMD_E, MDB_EXP_OPT_FTR_EN_SUBCMD, buf);
    printf("\n");
    MdbSetLevel(MDB_LEVEL_2);
    MdbSetExpansion(false);
}

static void TestBufPrint(const uint16_t *pucBuffer, uint8_t len)
{
    for (int i = 0; i < len; i++)
    {
        printf("0x%x ", *pucBuffer++);
    }
}
