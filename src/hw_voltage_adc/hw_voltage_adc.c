#include <stdio.h>

#include "hw_adc.h"

extern void UsartDebugSendString(const char *pucBuffer);

void HwVoltPrint(void)
{
	char buf[56];
    float u[3];

    u[0] = (BufADC[HW_ADC_12V] * 48.411 - 110810.7) / 4095;
    u[1] = (BufADC[HW_ADC_5V]  * 19.8 - 40540.5)    / 4095;
    u[2] = (BufADC[HW_ADC_3V3] * 19.8 - 40540.5)    / 4095;

    sprintf(buf, "Voltage:\r\n\tU1=%5.2fV\r\n\tU2=%5.2fV\r\n\tU3=%5.2fV\r\n", 
                u[0], u[1], u[2]);
    UsartDebugSendString(buf);
}