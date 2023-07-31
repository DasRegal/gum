#include <stdint.h>
#include <string.h>

#include "stm32f10x.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "coinbox.h"

static void CctalkUsartInit(void);

QueueHandle_t fdBuferCctalk;

void UART3InterruptHandler(void)
{
    if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
    {
        portBASE_TYPE true_false;
        uint8_t ch;

        ch = (uint8_t)USART_ReceiveData(USART3);
        USART_ClearITPendingBit(USART3, USART_IT_RXNE);
        xQueueSendFromISR(fdBuferCctalk, &ch, &true_false);
    }
}

void vTaskCctalk(void *pvParameters)
{
    uint8_t ch;
    QueueHandle_t fdBuferCctalk;
    fdBuferCctalk = (QueueHandle_t*)pvParameters;

    for(;;)
    {
        if (xQueueReceive(fdBuferCctalk, &ch, portMAX_DELAY) == pdPASS)
        {
            CctalkUartHandler(ch);
        }
    }
}

void CoinBoxInit(void)
{
    cctalk_master_dev_t dev;
    dev.send_data_cb = UsartSendString_Cctalk;

    fdBuferCctalk = xQueueCreate(256, sizeof(uint8_t));
    // cctalk_dev_t cctalk_dev;
    CctalkInit(dev);
    CctalkUsartInit();

    xTaskCreate(vTaskCctalk, (const char*)"cctalk", 256, (void*)fdBuferCctalk, tskIDLE_PRIORITY + 1, (TaskHandle_t*)NULL);
}

void CctalkUsartInit(void)
{
    USART_InitTypeDef USART_InitStructure;
    GPIO_InitTypeDef  GPIO_InitStructure;
    // DMA_InitTypeDef   DMA_InitStructure;
    NVIC_InitTypeDef  NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,  ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
    GPIO_PinRemapConfig(GPIO_PartialRemap_USART3, ENABLE);
    // RCC_AHBPeriphClockCmd ( RCC_AHBPeriph_DMA1,   ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOC, &GPIO_InitStructure );

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    // // Настройка DMA

    // DMA_DeInit(DMA1_Channel2);
    // DMA_InitStructure.DMA_PeripheralBaseAddr    = (uint32_t)&USART3->DR;
    // DMA_InitStructure.DMA_MemoryBaseAddr        = (uint32_t) cctalk_dev.tx_data;
    // DMA_InitStructure.DMA_DIR                   = DMA_DIR_PeripheralDST;
    // DMA_InitStructure.DMA_BufferSize            = CCTALK_MAX_BUF_LEN;
    // DMA_InitStructure.DMA_PeripheralInc         = DMA_PeripheralInc_Disable;
    // DMA_InitStructure.DMA_MemoryInc             = DMA_MemoryInc_Enable;
    // DMA_InitStructure.DMA_PeripheralDataSize    = DMA_PeripheralDataSize_HalfWord;
    // DMA_InitStructure.DMA_MemoryDataSize        = DMA_MemoryDataSize_HalfWord;
    // DMA_InitStructure.DMA_Mode                  = DMA_Mode_Circular;
    // DMA_InitStructure.DMA_Priority              = DMA_Priority_High;
    // DMA_InitStructure.DMA_M2M                   = DMA_M2M_Disable;
    // DMA_Init(DMA1_Channel2, &DMA_InitStructure);

    // DMA_Cmd(DMA1_Channel2, DISABLE);

    // USART_DMACmd(USART3, USART_DMAReq_Tx, DISABLE);

    USART_InitStructure.USART_BaudRate = 9600;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No ;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART3, &USART_InitStructure);

    // NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel2_IRQn;
    // NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = configLIBRARY_LOWEST_INTERRUPT_PRIORITY;
    // NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    // NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    // NVIC_Init(&NVIC_InitStructure);
    // DMA_ITConfig(DMA1_Channel2, DMA_IT_TC, ENABLE);

    // USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = configLIBRARY_LOWEST_INTERRUPT_PRIORITY;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(USART3, ENABLE);

    // for (int i = 0; i < 5000; i++);
    // // char buf[20];
    // // strcpy(buf, "Hello");
    // // UsartSendString_Cctalk(buf);

    // // Req equipment category id "Coin Acceptor"
    // char buf[10] = {2, 0, 1, 246, 7, '\n'};
    // buf[0] = 2;
    // buf[1] = 0;
    // buf[2] = 1;
    // buf[3] = 245;
    // buf[4] = 8;
    // for (char i = 0; i < 5; i++)
    // {
    // while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
    // USART_SendData(USART3, buf[i] );

    // }

//     // Req product code "SCA1"
// for (int i = 0; i < 50000; i++);
//      buf[0] = 2;
//     buf[1] = 0;
//     buf[2] = 1;
//     buf[3] = 244;
//     buf[4] = 9;
//     for (char i = 0; i < 5; i++)
//     {
//     while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
//     USART_SendData(USART3, buf[i] );

//     }

//     // Req build code "Standard"
// // for (int i = 0; i < 50000; i++);
    //  buf[0] = 2;
    // buf[1] = 0;
    // buf[2] = 1;
    // buf[3] = 192;
    // buf[4] = 61;
//     for (char i = 0; i < 5; i++)
//     {
//     while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
//     USART_SendData(USART3, buf[i] & 0x00FF );

//     }

//     // Serial number "C16B3D"
//     // Received data : [ serial 1 ] [ serial 2 ] [ serial 3 ]
//     // = [ serial 1 ] + 256 * [ serial 2 ] + 65536 * [ serial 3 ]
//     // 3997696 + 27392 + 193 = 4025281
// // for (int i = 0; i < 50000; i++);
//      buf[0] = 2;
//     buf[1] = 0;
//     buf[2] = 1;
//     buf[3] = 242;
//     buf[4] = 11;
//     for (char i = 0; i < 5; i++)
//     {
//     while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
//     USART_SendData(USART3, buf[i] & 0x00FF );

//     }

//     // Request software revision "SB10C"
// // for (int i = 0; i < 50000; i++);
//      buf[0] = 2;
//     buf[1] = 0;
//     buf[2] = 1;
//     buf[3] = 241;
//     buf[4] = 12;
//     for (char i = 0; i < 5; i++)
//     {
//     while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
//     USART_SendData(USART3, buf[i] );

//     }

//     // Request comms revision "0x01 0x04 0x02" - 1.4.2
// // for (int i = 0; i < 50000; i++);
//     buf[0] = 2;
//     buf[1] = 0;
//     buf[2] = 1;
//     buf[3] = 4;
//     buf[4] = 249;
//     for (char i = 0; i < 5; i++)
//     {
//         while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
//         USART_SendData(USART3, buf[i] );
//     }

    // // Request coin id for coin 1 = ""
    // buf[0] = 2;
    // buf[1] = 2;
    // buf[2] = 1;
    // buf[3] = 231;
    // buf[4] = 255;
    // buf[5] = 255;
    // buf[6] = 22;
    // for (char i = 0; i < 7; i++)
    // {
    //     while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
    //     USART_SendData(USART3, buf[i] );
    // }

    // for (int i = 0; i < 5000; i++);
    // UsartSendString_Cctalk(buf);

}

void UsartSendString_Cctalk(const char *pucBuffer)
{
    uint16_t i;

    USART_ITConfig(USART3, USART_IT_RXNE, DISABLE);
    for (i = 0; i < pucBuffer[1] + 5; i++, pucBuffer++)
    {
        while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
        USART_SendData(USART3, *pucBuffer & 0x00FF);
    }
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
}