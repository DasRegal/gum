#include <stdio.h>

#include "hw_adc.h"

#define HW_REV_ERR_VOLTAGE  10

extern void UsartDebugSendString(const char *pucBuffer);

static uint16_t hw_rev_table_x100[10] = 
{
    330,
    297,
    266,
    229,
    195,
    165,
    135,
    101,
    65,
    33
};

void HwRevPrint(void)
{
    char buf[28];
    uint32_t val[2];
    uint8_t maj = 0;
    uint8_t min = 0;

    val[0] = BufADC[HW_ADC_MAJ] * 330 / 4095;
    val[1] = BufADC[HW_ADC_MIN] * 330 / 4095;

    for (uint8_t i = 0; i < 10; i++)
    {
        if( (i != 0 ? (val[0] + HW_REV_ERR_VOLTAGE < hw_rev_table_x100[i - 1]) : 1) && 
            (i != 9 ? (val[0] - HW_REV_ERR_VOLTAGE > hw_rev_table_x100[i + 1]) : 1) )
        {
            maj = i;
        }

        if( (i != 0 ? (val[1] + HW_REV_ERR_VOLTAGE < hw_rev_table_x100[i - 1]) : 1) && 
            (i != 9 ? (val[1] - HW_REV_ERR_VOLTAGE > hw_rev_table_x100[i + 1]) : 1) )
        {
            min = i;
        }
    }

    sprintf(buf, "HW REV %d.%d\r\n", maj, min);
    UsartDebugSendString(buf);
}
