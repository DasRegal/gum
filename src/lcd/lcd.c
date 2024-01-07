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
#include "cashless.h"
#include "satellite.h"

#define LCD_BUF_LEN     0xFF

void vTaskLcdBuf(void *pvParameters);
void vTaskLcdButton(void *pvParameters);

void vTimerItemSelTimeoutCb( TimerHandle_t xTimer );

static void LcdUartInit(void);
static void LcdSendString(const char *pucBuffer, uint8_t len);
static void LcdUpdateBalance(uint32_t balance);
EventGroupHandle_t  xLcdButtonEventGroup;
EventGroupHandle_t  xLcdVendItemGroup;
TimerHandle_t xItemSelTimer;

char lcdBuf[LCD_BUF_LEN];

volatile uint32_t vend_balance;
volatile uint32_t price;
volatile uint32_t item;

static bool is_vend;

typedef struct
{
    char *buf;
    uint8_t index;
} lcd_dev_t;

lcd_dev_t lcd_dev = { .buf = lcdBuf, .index = 0 };

QueueHandle_t fdLcdBuf;

void LcdInit(void)
{
    vend_balance = 0;
    price = 0;
    item = 0;
    is_vend = 0;
    DwinInit(LcdSendString);
    LcdUartInit();

    fdLcdBuf = xQueueCreate(LCD_BUF_LEN, sizeof(char));
    xItemSelTimer = xTimerCreate ( "SelTimeout2", 30000, pdFALSE, ( void * ) 0, vTimerItemSelTimeoutCb );

    xTaskCreate(vTaskLcdBuf, (const char*)"Lcd", 256, NULL, tskIDLE_PRIORITY + 2, (TaskHandle_t*)NULL);
    xTaskCreate(vTaskLcdButton, (const char*)"Lcd B", 256, NULL, tskIDLE_PRIORITY + 2, (TaskHandle_t*)NULL);
}

void vTaskLcdBuf(void *pvParameters)
{
    char ch;

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
    uint16_t button;
    char b1[6] = { 0x00, 0x00 , 0x00, 0x00, 0x00, 0x00 };
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
                price = 120;
                xEventGroupSetBits(xLcdButtonEventGroup, (1 << 0));
                DwinSetPage(2);
                CashlessVendRequest(price, 1);
                item = 0;
            }

            if((button & 0xff) == 1)
            {
                price = 50;
                xEventGroupSetBits(xLcdButtonEventGroup, (1 << 1));
                DwinSetPage(2);
                CashlessVendRequest(price, 2);
                item = 1;
            }

            if((button & 0xff) == 2)
            {
                price = 1;
                xEventGroupSetBits(xLcdButtonEventGroup, (1 << 2));
                DwinSetPage(2);
                CashlessVendRequest(price, 3);
                // price = 15;
                item = 2;
            }

            if((button & 0xff) == 3)
            {
                price = 40;
                xEventGroupSetBits(xLcdButtonEventGroup, (1 << 3));
                DwinSetPage(2);
                CashlessVendRequest(price, 4);
                item = 3;
            }

            xTimerStart(xItemSelTimer, 0);

            DwinHandleButton(button);
        }

        if (CashlessIsVendApproved(NULL))
        {
            xTimerStop(xItemSelTimer, 0);

            is_vend = true;
            DwinSetPage(3);
            vend_balance += price;

            LcdUpdateBalance(vend_balance);

            uint32_t p_x100;
            uint32_t p_x10;
            uint32_t p_x1;

            p_x100 = vend_balance / 100;
            p_x10  = (vend_balance - p_x100 * 100) / 10;
            p_x1   = (vend_balance - p_x100 * 100 - p_x10 * 10);

            b1[0] = 0;
            b1[1] = 0x30 + p_x100;
            b1[2] = 0;
            b1[3] = 0x30 + p_x10;
            b1[4] = 0;
            b1[5] = 0x30 + p_x1;
            LcdWrite(0x2000, b1, 6);

            xEventGroupSetBits(xLcdVendItemGroup, (1 << item));
            SatVend(item);
        }

        if (is_vend)
        {
            uint8_t res = SatIsVendOk();
            switch(res)
            {
                case 1:
                    CashlessVendSuccessCmd(0);
                    if (vend_balance >= price)
                        vend_balance -= price;
                    else
                        vend_balance = 0;
                    LcdUpdateBalance(vend_balance);
                    is_vend = false;
                    DwinSetPage(1);
                    break;
                case 2:
                    CashlessVendFailureCmd();
                    LcdUpdateBalance(vend_balance);
                    is_vend = false;
                    DwinSetPage(1);
                    break;
                case 0:
                default:
                    break;
            }
        }
        vTaskDelay(100);
    }
}

static void LcdUpdateBalance(uint32_t balance)
{
    char buf[6] = { 0x00, 0x00 , 0x00, 0x00, 0x00, 0x00 };
    uint32_t p_x100;
    uint32_t p_x10;
    uint32_t p_x1;

    p_x100 = balance / 100;
    p_x10  = (balance - p_x100 * 100) / 10;
    p_x1   = (balance - p_x100 * 100 - p_x10 * 10);

    buf[0] = 0;
    buf[1] = 0x30 + p_x100;
    buf[2] = 0;
    buf[3] = 0x30 + p_x10;
    buf[4] = 0;
    buf[5] = 0x30 + p_x1;
    LcdWrite(0x2000, buf, 6);
}

void vTimerItemSelTimeoutCb( TimerHandle_t xTimer )
{
    xTimerStop(xItemSelTimer, 0);
    SatResume();
    DwinSetPage(1);
    CashlessVendCancelCmd();
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
