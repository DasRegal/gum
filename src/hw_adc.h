#ifndef _HW_ADC_H
#define _HW_ADC_H

extern uint16_t BufADC[5];

enum
{
	HW_ADC_MAJ,
	HW_ADC_MIN,
	HW_ADC_12V,
	HW_ADC_5V,
	HW_ADC_3V3
};

void HwAdcInit(void);

#endif /* _HW_ADC_H */