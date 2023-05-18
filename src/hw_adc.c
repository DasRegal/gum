#include "stm32f10x.h"

uint16_t BufADC[5];

void HwAdcInit(void)
{
	GPIO_InitTypeDef    GPIO_InitStructure = { .GPIO_Speed = GPIO_Speed_2MHz,
                                               .GPIO_Mode  = GPIO_Mode_AIN };
    DMA_InitTypeDef     DMA_InitStructure;
    ADC_InitTypeDef     ADC_InitStructure;
    // Тактирование
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_AFIO,    ENABLE);
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_ADC1,    ENABLE);
    RCC_AHBPeriphClockCmd ( RCC_AHBPeriph_DMA1,     ENABLE);
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOB,   ENABLE);
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOC,   ENABLE);

    RCC_ADCCLKConfig (RCC_PCLK2_Div6);

    // Настройка портов
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    // Настройка DMA

    DMA_DeInit(DMA1_Channel1);
    DMA_InitStructure.DMA_PeripheralBaseAddr    = (uint32_t)&ADC1->DR;
    DMA_InitStructure.DMA_MemoryBaseAddr        = (uint32_t) BufADC;
    DMA_InitStructure.DMA_DIR                   = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize            = 5;
    DMA_InitStructure.DMA_PeripheralInc         = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc             = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize    = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize        = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode                  = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority              = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M                   = DMA_M2M_Disable;
    DMA_InitStructure.DMA_PeripheralDataSize    = DMA_PeripheralDataSize_HalfWord;
    DMA_Init(DMA1_Channel1, &DMA_InitStructure);
    DMA_Cmd(DMA1_Channel1, ENABLE);

    // настройки ADC
    ADC_StructInit(&ADC_InitStructure);
    ADC_InitStructure.ADC_Mode                  = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode          = ENABLE;
    ADC_InitStructure.ADC_ContinuousConvMode    = ENABLE;
    ADC_InitStructure.ADC_ExternalTrigConv      = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign             = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel          = 5;
    ADC_Init(ADC1, &ADC_InitStructure);
    ADC_Cmd(ADC1, ENABLE);

    // настройка канала
    ADC_RegularChannelConfig(ADC1, ADC_Channel_8, 1, ADC_SampleTime_7Cycles5);
    ADC_RegularChannelConfig(ADC1, ADC_Channel_9, 2, ADC_SampleTime_7Cycles5);
    ADC_RegularChannelConfig(ADC1, ADC_Channel_12, 3, ADC_SampleTime_7Cycles5);
    ADC_RegularChannelConfig(ADC1, ADC_Channel_11, 4, ADC_SampleTime_7Cycles5);
    ADC_RegularChannelConfig(ADC1, ADC_Channel_10, 5, ADC_SampleTime_7Cycles5);

    ADC_DMACmd(ADC1, ENABLE);

    // калибровка АЦП
    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1));

    ADC_SoftwareStartConvCmd(ADC1, ENABLE);

    for(int i = 0; i < 100; i++)
    {
    	asm("NOP");
    }
}