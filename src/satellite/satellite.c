#include "stm32f10x.h"

#include "FreeRTOS.h"
#include "event_groups.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

#include "main.h"
#include "vsp.h"
#include "satellite.h"
#include "dwin.h"

/* Latch In */
#define SAT_LATCH_IN_PIN    GPIO_Pin_4
#define SAT_LATCH_IN_PORT   GPIOC
#define SAT_LATCH_IN_RCC    RCC_APB2Periph_GPIOC
/* Latch Out */
#define SAT_LATCH_OUT_PIN   GPIO_Pin_3
#define SAT_LATCH_OUT_PORT  GPIOC
#define SAT_LATCH_OUT_RCC   RCC_APB2Periph_GPIOC
/* CS1 */
#define SAT_CS1_PIN         GPIO_Pin_4
#define SAT_CS1_PORT        GPIOB
#define SAT_CS1_RCC         RCC_APB2Periph_GPIOB
/* CS2 */
#define SAT_CS2_PIN         GPIO_Pin_6
#define SAT_CS2_PORT        GPIOB
#define SAT_CS2_RCC         RCC_APB2Periph_GPIOB
/* CS3 */
#define SAT_CS3_PIN         GPIO_Pin_7
#define SAT_CS3_PORT        GPIOB
#define SAT_CS3_RCC         RCC_APB2Periph_GPIOB
/* CS4 */
#define SAT_CS4_PIN         GPIO_Pin_14
#define SAT_CS4_PORT        GPIOC
#define SAT_CS4_RCC         RCC_APB2Periph_GPIOC
/* INT */
#define SAT_INT_PIN         GPIO_Pin_3
#define SAT_INT_PORT        GPIOB
#define SAT_INT_RCC         RCC_APB2Periph_GPIOB
#define SAT_INT_PORT_SRC    GPIO_PortSourceGPIOB
#define SAT_INT_PIN_SRC     GPIO_PinSource3
#define SAT_INT_EXTI_LINE   EXTI_Line3
#define SAT_INT_EXTI_IRQN   EXTI3_IRQn
/* BUT */
#define SAT_BUT_PIN         GPIO_Pin_5
#define SAT_BUT_PORT        GPIOB
#define SAT_BUT_RCC         RCC_APB2Periph_GPIOB
/* MISO */
#define SAT_MISO_PIN        GPIO_Pin_14
#define SAT_MISO_PORT       GPIOB
#define SAT_MISO_RCC        RCC_APB2Periph_GPIOB
/* MOSI */
#define SAT_MOSI_PIN        GPIO_Pin_15
#define SAT_MOSI_PORT       GPIOB
#define SAT_MOSI_RCC        RCC_APB2Periph_GPIOB
/* CLK */
#define SAT_CLK_PIN         GPIO_Pin_13
#define SAT_CLK_PORT        GPIOB
#define SAT_CLK_RCC         RCC_APB2Periph_GPIOB

SemaphoreHandle_t   sat_block_poll_sem;
TaskHandle_t        ledstream_handler;
SemaphoreHandle_t   xItemMutex;
SemaphoreHandle_t   xVendMutex;
extern EventGroupHandle_t  xLcdButtonEventGroup;
extern EventGroupHandle_t  xLcdVendItemGroup;
static uint8_t is_vend;

static void SatLatchOut(bool enable);
static void SatLatchIn(bool enable);
static void SatCs1(bool enable);
static void SatCs2(bool enable);
static void SatCs3(bool enable);
static void SatCs4(bool enable);
static void SatClk(bool enable);
static void SatInitPeriph(void);
static bool SatSens(void);
static bool SatButton(void);

void vTaskSatellitePoll (void *pvParameters);
void vTaskLedStream (void *pvParameters);

void SatInit(void)
{
    is_vend = 0;
    vsp_dev_t vsp_dev;

    vsp_dev.cb_latch_out    = SatLatchOut;
    vsp_dev.cb_latch_in     = SatLatchIn;
    vsp_dev.item[0].cb_cs   = SatCs1;
    vsp_dev.item[1].cb_cs   = SatCs2;
    vsp_dev.item[2].cb_cs   = SatCs3;
    vsp_dev.item[3].cb_cs   = SatCs4;
    vsp_dev.cb_clk          = SatClk;
    vsp_dev.cb_sat_sens     = SatSens;
    vsp_dev.cb_sat_button   = SatButton;

    VspInit(vsp_dev);
    SatInitPeriph();

    vSemaphoreCreateBinary(sat_block_poll_sem);
    xSemaphoreGive(sat_block_poll_sem);
    xItemMutex = xSemaphoreCreateMutex();
    xVendMutex = xSemaphoreCreateMutex();
    xLcdButtonEventGroup = xEventGroupCreate();
    xLcdVendItemGroup = xEventGroupCreate();

    xTaskCreate(
        vTaskSatellitePoll,
        "Satellite",
        300,
        (void*)NULL,
        tskIDLE_PRIORITY + 2,
        NULL
    );

    xTaskCreate(
        vTaskLedStream,
        "LedStream",
        300,
        (void*)NULL,
        tskIDLE_PRIORITY + 2,
        &ledstream_handler
    );
}

void vTaskSatellitePoll (void *pvParameters)
{
    SatCs1(false);
    SatCs2(false);
    SatCs3(false);
    SatCs4(false);
    VspEnable(0, true);
    VspEnable(1, true);
    VspEnable(2, true);
    VspEnable(3, true);
    EventBits_t flags;
    // VspBlock(3, true);
    for( ;; )
    {
        xSemaphoreTake(sat_block_poll_sem, portMAX_DELAY);
        // if (xSemaphoreTake(sat_block_poll_sem, 60000) == pdPASS)
        // {
            uint8_t item = VspGetSelectItem();
            if(item < VSP_MAX_ITEMS)
            {
                xSemaphoreTake(xVendMutex, portMAX_DELAY);
                is_vend = 1;
                xSemaphoreGive(xVendMutex);

                // DwinSetPage(1);

                xSemaphoreTake(xItemMutex, portMAX_DELAY);
                    VspMotorCtrl(item, false);

                    VspInhibitCtrl(item, false);
                    for (size_t idx = 0; idx < VSP_MAX_ITEMS; idx++)
                    {
                        VspDeselectItem(idx);
                    }
                xSemaphoreGive(xItemMutex);

                vTaskResume(ledstream_handler);
            }

            for (size_t idx = 0; idx < VSP_MAX_ITEMS; idx++)
            {
                xSemaphoreTake(xItemMutex, portMAX_DELAY);
                    VspCheck(idx);
                xSemaphoreGive(xItemMutex);
            }

            /* Start from buttons */
            // for (size_t idx = 0; idx < VSP_MAX_ITEMS; idx++)
            // {
            //     xSemaphoreTake(xItemMutex, portMAX_DELAY);
            //     bool is_push = VspButton(idx);
            //     xSemaphoreGive(xItemMutex);
            //     if (is_push)
            //     {
            //         vTaskSuspend(ledstream_handler);
            //         xSemaphoreTake(xItemMutex, portMAX_DELAY);
            //             VspSelectItem(idx);
            //         xSemaphoreGive(xItemMutex);

            //         // xSemaphoreTake(xItemMutex, portMAX_DELAY);
            //         // VspInhibitCtrl(idx, true);
            //         // VspMotorCtrl(idx, true);
            //         // xSemaphoreGive(xItemMutex);

            //         break;
            //     }
            // }

            /* Start from LCD buttons */
            flags = xEventGroupGetBits(xLcdButtonEventGroup);
            if (flags)
            {
                for (size_t idx = 0; idx < VSP_MAX_ITEMS; idx++)
                {
                    if ( flags & (1 << idx))
                    {
                        xEventGroupClearBits(xLcdButtonEventGroup, (1 << idx));
                        
                        vTaskSuspend(ledstream_handler);

                        xSemaphoreTake(xItemMutex, portMAX_DELAY);
                            VspSelectItem(idx);
                        xSemaphoreGive(xItemMutex);

                        // xSemaphoreTake(xItemMutex, portMAX_DELAY);
                        // VspInhibitCtrl(idx, true);
                        // VspMotorCtrl(idx, true);
                        // xSemaphoreGive(xItemMutex);

                    break;
                    }
                }
            }


            /* If pushed the button */
            if(VspGetSelectItem() < VSP_MAX_ITEMS)
            {
                continue;
            }

            vTaskDelay(100);
            xSemaphoreGive(sat_block_poll_sem);
        // }
        // else
        // {
        //     xSemaphoreTake(xVendMutex, portMAX_DELAY);
        //         is_vend = 2;
        //     xSemaphoreGive(xVendMutex);


        //     // uint8_t item = VspGetSelectItem();
        //     // xSemaphoreTake(xItemMutex, portMAX_DELAY);
        //     // VspInhibitCtrl(item, false);
        //     // SatContinuePoll();
        //     // xSemaphoreGive(xItemMutex);
        //     // vTaskResume(ledstream_handler);

        //     // DwinSetPage(1);

        //     xSemaphoreTake(xItemMutex, portMAX_DELAY);
        //         uint8_t item = VspGetSelectItem();
        //         VspMotorCtrl(item, false);

        //         VspInhibitCtrl(item, false);
        //         for (size_t idx = 0; idx < VSP_MAX_ITEMS; idx++)
        //         {
        //             VspDeselectItem(idx);
        //         }
        //     xSemaphoreGive(xItemMutex);
            
        //     vTaskResume(ledstream_handler);
            
        //     xSemaphoreGive(sat_block_poll_sem);
        // }

    }
}

void SatPushLcdButton(uint8_t button)
{
    xEventGroupSetBits(xLcdButtonEventGroup, (1 << button));
}

bool SatIsPushButton(uint16_t *button)
{
    for (size_t idx = 0; idx < VSP_MAX_ITEMS; idx++)
    {
        xSemaphoreTake(xItemMutex, portMAX_DELAY);
        bool is_push = VspButton(idx);
        xSemaphoreGive(xItemMutex);
        if (is_push)
        {
            *button = idx;
            return true;
        }
    }
    return false;
}

void SatVendTimeout(void)
{
    xSemaphoreTake(xVendMutex, portMAX_DELAY);
        is_vend = 2;
    xSemaphoreGive(xVendMutex);

    xSemaphoreTake(xItemMutex, portMAX_DELAY);
        uint8_t item = VspGetSelectItem();
        VspMotorCtrl(item, false);

        VspInhibitCtrl(item, false);
        for (size_t idx = 0; idx < VSP_MAX_ITEMS; idx++)
        {
            VspDeselectItem(idx);
        }
    xSemaphoreGive(xItemMutex);
    
    vTaskResume(ledstream_handler);
    
    xSemaphoreGive(sat_block_poll_sem);
}

// void SatResume(void)
// {
//     for (size_t idx = 0; idx < VSP_MAX_ITEMS; idx++)
//     {
//         VspDeselectItem(idx);
//     }
//     xSemaphoreGive(xItemMutex);
//     vTaskResume(ledstream_handler);
    
//     xSemaphoreGive(sat_block_poll_sem);
// }

void SatVend(uint8_t item)
{
    // EventBits_t flags;
    // flags = xEventGroupGetBits(xLcdVendItemGroup);
    // if (flags)
    // {
    //     for (size_t idx = 0; idx < VSP_MAX_ITEMS; idx++)
    //     {
    //         if ( flags & (1 << idx))
    //         {
                // xEventGroupClearBits(xLcdVendItemGroup, (1 << idx));
                
                xSemaphoreTake(xItemMutex, portMAX_DELAY);
                    VspInhibitCtrl(item, true);
                    VspMotorCtrl(item, true);
                xSemaphoreGive(xItemMutex);
    //         break;
    //         }
    //     }
    // }
}

uint8_t SatIsVendOk(void)
{
    uint8_t ret;
    xSemaphoreTake(xVendMutex, portMAX_DELAY);
    ret =  is_vend;
    is_vend = 0;
    xSemaphoreGive(xVendMutex);
    return ret;
}

void vTaskLedStream (void *pvParameters)
{
    #define SAT_LEDSTREAM_SPEED_1 80
    #define SAT_LEDSTREAM_SPEED_2 720
    for ( ;; )
    {
        xSemaphoreTake(xItemMutex, portMAX_DELAY);
        VspLedCtrl(0, true);
        xSemaphoreGive(xItemMutex);

        vTaskDelay(SAT_LEDSTREAM_SPEED_1);

        xSemaphoreTake(xItemMutex, portMAX_DELAY);
        VspLedCtrl(3, false);
        xSemaphoreGive(xItemMutex);

        vTaskDelay(SAT_LEDSTREAM_SPEED_2);

        xSemaphoreTake(xItemMutex, portMAX_DELAY);
        VspLedCtrl(1, true);
        xSemaphoreGive(xItemMutex);

        vTaskDelay(SAT_LEDSTREAM_SPEED_1);

        xSemaphoreTake(xItemMutex, portMAX_DELAY);
        VspLedCtrl(0, false);
        xSemaphoreGive(xItemMutex);

        vTaskDelay(SAT_LEDSTREAM_SPEED_2);

        xSemaphoreTake(xItemMutex, portMAX_DELAY);
        VspLedCtrl(2, true);
        xSemaphoreGive(xItemMutex);

        vTaskDelay(SAT_LEDSTREAM_SPEED_1);

        xSemaphoreTake(xItemMutex, portMAX_DELAY);
        VspLedCtrl(1, false);
        xSemaphoreGive(xItemMutex);

        vTaskDelay(SAT_LEDSTREAM_SPEED_2);

        xSemaphoreTake(xItemMutex, portMAX_DELAY);
        VspLedCtrl(3, true);
        xSemaphoreGive(xItemMutex);

        vTaskDelay(SAT_LEDSTREAM_SPEED_1);

        xSemaphoreTake(xItemMutex, portMAX_DELAY);
        VspLedCtrl(2, false);
        xSemaphoreGive(xItemMutex);

        vTaskDelay(SAT_LEDSTREAM_SPEED_2);
    }
}

void SatContinuePoll(void)
{
    // // if(VspGetSelectItem() == VSP_MAX_ITEMS)
    // //     return;

    // for (size_t idx = 0; idx < VSP_MAX_ITEMS; idx++)
    // {
    //     VspDeselectItem(idx);
    // }
    // xSemaphoreGive(sat_block_poll_sem);
}

uint8_t SatGetPushButton(void)
{
    return VspGetSelectItem();
}

static void SatInitPeriph(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure = { .NVIC_IRQChannel                   = SPI2_IRQn,
                                            .NVIC_IRQChannelPreemptionPriority = 0xFF,
                                            .NVIC_IRQChannelSubPriority        = 0,
                                            .NVIC_IRQChannelCmd                = ENABLE};

    // SPI_InitTypeDef SPI_InitStructure   = { .SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256,
    //                                         .SPI_CPHA              = SPI_CPHA_1Edge,
    //                                         .SPI_CPOL              = SPI_CPOL_Low,
    //                                         .SPI_DataSize          = SPI_DataSize_8b,
    //                                         .SPI_Direction         = SPI_Direction_2Lines_FullDuplex,
    //                                         .SPI_FirstBit          = SPI_FirstBit_MSB,
    //                                         .SPI_Mode              = SPI_Mode_Master,
    //                                         .SPI_NSS               = SPI_NSS_Soft,
    //                                         .SPI_CRCPolynomial     = 7};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB2PeriphClockCmd( SAT_MOSI_RCC        |
                            SAT_MISO_RCC        |
                            SAT_CLK_RCC, ENABLE);
    RCC_APB2PeriphClockCmd( SAT_LATCH_IN_RCC    |
                            SAT_LATCH_OUT_RCC   |
                            SAT_CS1_RCC         |
                            SAT_CS2_RCC         |
                            SAT_CS3_RCC         |
                            SAT_CS4_RCC         |
                            SAT_INT_RCC         |
                            SAT_BUT_RCC, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

    /* Latch IN */
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = SAT_LATCH_IN_PIN;
    GPIO_Init(SAT_LATCH_IN_PORT, &GPIO_InitStructure);
    /* Latch OUT */
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = SAT_LATCH_OUT_PIN;
    GPIO_Init(SAT_LATCH_OUT_PORT, &GPIO_InitStructure);
    SatLatchIn(false);
    SatLatchOut(true);
    /* CLK */
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    // GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = SAT_CLK_PIN;
    GPIO_Init(SAT_CLK_PORT, &GPIO_InitStructure);
    /* MISO */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = SAT_MISO_PIN;
    GPIO_Init(SAT_MISO_PORT, &GPIO_InitStructure);
    /* MOSI */
    // GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = SAT_MOSI_PIN;
    GPIO_Init(SAT_MOSI_PORT, &GPIO_InitStructure);
    /* CS1 */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = SAT_CS1_PIN;
    GPIO_Init(SAT_CS1_PORT, &GPIO_InitStructure);
    /* CS2 */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = SAT_CS2_PIN;
    GPIO_Init(SAT_CS2_PORT, &GPIO_InitStructure);
    /* CS3 */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = SAT_CS3_PIN;
    GPIO_Init(SAT_CS3_PORT, &GPIO_InitStructure);
    /* CS4 */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = SAT_CS4_PIN;
    GPIO_Init(SAT_CS4_PORT, &GPIO_InitStructure);

    GPIO_SetBits(SAT_CS1_PORT, SAT_CS1_PIN);
    GPIO_SetBits(SAT_CS2_PORT, SAT_CS2_PIN);
    GPIO_SetBits(SAT_CS3_PORT, SAT_CS3_PIN);
    GPIO_SetBits(SAT_CS4_PORT, SAT_CS4_PIN);

    /* BUTTON */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = SAT_BUT_PIN;
    GPIO_Init(SAT_BUT_PORT, &GPIO_InitStructure);
    /* INT */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Pin = SAT_INT_PIN;
    GPIO_Init(SAT_INT_PORT, &GPIO_InitStructure);

    GPIO_SetBits(SAT_CS1_PORT, SAT_CS1_PIN);
    GPIO_SetBits(SAT_CS2_PORT, SAT_CS2_PIN);
    GPIO_SetBits(SAT_CS3_PORT, SAT_CS3_PIN);
    GPIO_SetBits(SAT_CS4_PORT, SAT_CS4_PIN);

    // SPI_Init(SPI2, &SPI_InitStructure);
    // SPI_Cmd(SPI2, ENABLE);

    GPIO_EXTILineConfig(SAT_INT_PORT_SRC, SAT_INT_PIN_SRC);

    NVIC_Init(&NVIC_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel                      = SAT_INT_EXTI_IRQN;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority    = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority           = 0x00;
    NVIC_InitStructure.NVIC_IRQChannelCmd                   = ENABLE;
    NVIC_Init(&NVIC_InitStructure);


    EXTI_InitTypeDef EXTI_InitStructure;
    EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Line    = SAT_INT_EXTI_LINE;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
}

static void SatLatchOut(bool enable)
{
    if (enable)
        GPIO_SetBits(SAT_LATCH_OUT_PORT, SAT_LATCH_OUT_PIN);
    else
        GPIO_ResetBits(SAT_LATCH_OUT_PORT, SAT_LATCH_OUT_PIN);
}

static void SatLatchIn(bool enable)
{
    if (enable)
        GPIO_SetBits(SAT_LATCH_IN_PORT, SAT_LATCH_IN_PIN);
    else
        GPIO_ResetBits(SAT_LATCH_IN_PORT, SAT_LATCH_IN_PIN);
}

static void SatCs1(bool enable)
{
    if (enable)
    {
        GPIO_SetBits(SAT_CS1_PORT, SAT_CS1_PIN);
        vTaskDelay(1);
    }
    else
        GPIO_ResetBits(SAT_CS1_PORT, SAT_CS1_PIN);
}

static void SatCs2(bool enable)
{
    if (enable)
    {
        GPIO_SetBits(SAT_CS2_PORT, SAT_CS2_PIN);
        vTaskDelay(1);
    }
    else
        GPIO_ResetBits(SAT_CS2_PORT, SAT_CS2_PIN);
}

static void SatCs3(bool enable)
{
    if (enable)
    {
        GPIO_SetBits(SAT_CS3_PORT, SAT_CS3_PIN);
        vTaskDelay(1);
    }
    else
        GPIO_ResetBits(SAT_CS3_PORT, SAT_CS3_PIN);
}

static void SatCs4(bool enable)
{
    if (enable)
    {
        GPIO_SetBits(SAT_CS4_PORT, SAT_CS4_PIN);
        vTaskDelay(1);
    }
    else
        GPIO_ResetBits(SAT_CS4_PORT, SAT_CS4_PIN);
}

static void SatClk(bool enable)
{
    if (enable)
        GPIO_SetBits(SAT_CLK_PORT, SAT_CLK_PIN);
    else
        GPIO_ResetBits(SAT_CLK_PORT, SAT_CLK_PIN);
}

static bool SatSens(void)
{
    uint8_t stat;

    stat = GPIO_ReadInputDataBit(SAT_MISO_PORT, SAT_MISO_PIN);

    return stat ? false : true;
}

static bool SatButton(void)
{
    uint8_t stat;

    stat = GPIO_ReadInputDataBit(SAT_BUT_PORT, SAT_BUT_PIN);

    return stat ? false : true;
}

void EXTI3_IRQHandler(void)
{
    static  portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    
    if (EXTI_GetITStatus(EXTI_Line3))
    {
        EXTI_ClearITPendingBit(EXTI_Line3);
        xSemaphoreGiveFromISR(sat_block_poll_sem, &xHigherPriorityTaskWoken);
        if(xHigherPriorityTaskWoken == pdTRUE) taskYIELD();
    }
}
