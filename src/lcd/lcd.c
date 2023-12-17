#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "stm32f10x.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"
#include "event_groups.h"

#include "main.h"
#include "lcd.h"
#include "dwin.h"

#define LCD_BUF_LEN     0xFF

void vTaskLcdBuf(void *pvParameters);
void vTaskLcdButton(void *pvParameters);

static void LcdUartInit(void);
static void LcdSendString(const char *pucBuffer, uint8_t len);
extern EventGroupHandle_t  xLcdButtonEventGroup;

char lcdBuf[LCD_BUF_LEN];

typedef struct
{
    char *buf;
    uint8_t index;
} lcd_dev_t;

lcd_dev_t lcd_dev = { .buf = lcdBuf, .index = 0 };

QueueHandle_t fdLcdBuf;

void LcdInit(void)
{
    DwinInit(LcdSendString);
    LcdUartInit();

    fdLcdBuf = xQueueCreate(LCD_BUF_LEN, sizeof(char));

    xTaskCreate(vTaskLcdBuf, (const char*)"Lcd", 256, NULL, tskIDLE_PRIORITY + 2, (TaskHandle_t*)NULL);
    xTaskCreate(vTaskLcdButton, (const char*)"Lcd B", 256, NULL, tskIDLE_PRIORITY + 2, (TaskHandle_t*)NULL);
}

void vTaskLcdBuf(void *pvParameters)
{
    char ch;
    uint16_t button;
    char s[20];
    char b1[2] = { 0x00, 0x00 };

    for(;;)
    {
        if (xQueueReceive(fdLcdBuf, &ch, portMAX_DELAY) == pdPASS)
        {
            DwinGetCharHandler(ch);
        }
    }
}

void vTaskLcdButton(void *pvParameters)
{
    char ch;
    uint16_t button;
    char s[20];
    char b1[4] = { 0x00, 0x00 , 0x00, 0x00 };
    DwinReset();
    vTaskDelay(1000);
    DwinSetPage(1);
    // vTaskDelay(500);
    for(;;)
    {
        if (DwinIsPushButton(&button))
        {
            if((button & 0xff) == 0)
            {
                xEventGroupSetBits(xLcdButtonEventGroup, (1 << 0));
                DwinSetPage(3);
                // DwinButtonEn(0x1002, true);
            }
            if((button & 0xff) == 1)
            {
                xEventGroupSetBits(xLcdButtonEventGroup, (1 << 1));
                DwinSetPage(3);
                // DwinButtonEn(0x1002, false);
            }

            if((button & 0xff) == 2)
            {
                xEventGroupSetBits(xLcdButtonEventGroup, (1 << 2));
                DwinSetPage(3);
                // DwinButtonEn(0x1002, true);
            }
            if((button & 0xff) == 3)
            {
                xEventGroupSetBits(xLcdButtonEventGroup, (1 << 3));
                DwinSetPage(3);
                // DwinButtonEn(0x1002, false);
            }
            // if((button & 0xff) == 2)
            // {
            //     b1[0] = 0x5A;
            //     b1[1] = 0x01;
            //     b1[2] = 0x00;
            //     b1[3] = 0x01;
            //     LcdWrite(0x0084, b1, 4);
            // }
            // sprintf(s, "\rPush button %d\r\n", button & 0xff);
            // PRINT_OS(s);
            DwinHandleButton(button);
        }
        vTaskDelay(100);
    }
}


void LcdWrite(uint16_t vp, char *data, uint8_t len)
{
    char buf[6] = { 0x5A, 0xA5, 0x00, 0x82, 0x00, 0x00 };

    if (len > LCD_BUF_LEN - 1)
        return;

    buf[2] = len + 3;
    buf[4] = (char)((vp >> 8) & 0xFF);
    buf[5] = (char)(vp & 0xFF);
    LcdSendString(buf, 6);

    LcdSendString(data, len);
}

static void LcdSendString(const char *pucBuffer, uint8_t len)
{
    char c;
    while(len--)
    {
        c = *pucBuffer++;
        while(USART_GetFlagStatus(UART5, USART_FLAG_TXE) == RESET);
        USART_SendData(UART5, c);
    }
}

static void LcdUartInit(void)
{
    USART_InitTypeDef USART_InitStructure;
    GPIO_InitTypeDef  GPIO_InitStructure;
    NVIC_InitTypeDef  NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,  ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD,  ENABLE);

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART5, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOD, &GPIO_InitStructure );

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No ;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(UART5, &USART_InitStructure);

    USART_ITConfig(UART5, USART_IT_RXNE, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = UART5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = configLIBRARY_LOWEST_INTERRUPT_PRIORITY;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(UART5, ENABLE);
}

void UART5InterruptHandler(void)
{
    if(USART_GetITStatus(UART5, USART_IT_RXNE) != RESET)
    {
        char ch;

        ch = (char)USART_ReceiveData(UART5);
        USART_ClearITPendingBit(UART5, USART_IT_RXNE);

        xQueueSendFromISR(fdLcdBuf, &ch, NULL);
    }
}