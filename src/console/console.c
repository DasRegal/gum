#include "stm32f10x.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

#include "console.h"
#include "rcli/rcli.h"
#include "rcli/rcli.h"

volatile    uint8_t     TxIndex = 0;
volatile    uint8_t*    pTxData;
QueueHandle_t fdBuferConsoleRec;

void UART1InterruptHandler(void)
{
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        portBASE_TYPE tru_false;
        uint8_t ch;

        ch = (uint8_t)USART_ReceiveData(USART1);
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
        xQueueSendFromISR(fdBuferConsoleRec, &ch, &tru_false);
    }
}

void vTaskConsoleRecBuf(void *pvParameters)
{
    uint8_t ch;
    QueueHandle_t fdBuferConsoleRec;
    fdBuferConsoleRec = (QueueHandle_t*)pvParameters;

    for(;;)
    {
        if (xQueueReceive(fdBuferConsoleRec, &ch, portMAX_DELAY) == pdPASS)
        {
            RcliUartHandler(ch);
        }
    }
}

void ConsoleInit(void)
{
    fdBuferConsoleRec = xQueueCreate(512, sizeof(uint8_t));

    USART_InitTypeDef USART_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure );

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);


    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No ;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = configLIBRARY_LOWEST_INTERRUPT_PRIORITY;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(USART1, ENABLE);

    xTaskCreate(vTaskConsoleRecBuf, (const char*)"Console", 256, (void*)fdBuferConsoleRec, tskIDLE_PRIORITY + 1, (TaskHandle_t*)NULL);
}

void ConsoleCliStart(void)
{
    RcliInit();
}

void UsartDebugSendString(const char *pucBuffer)
{
    char c;
    while((c = *pucBuffer++))
    {
        while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        USART_SendData(USART1, c & 0x00FF);
    }
}
