#include "stm32f10x.h"

#include "main.h"
#include "vsp.h"
#include "satellite.h"

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
#define SAT_CLK_PIN         GPIO_Pin_12
#define SAT_CLK_PORT        GPIOB
#define SAT_CLK_RCC         RCC_APB2Periph_GPIOB

static void SatLatchOut(bool enable);
static void SatLatchIn(bool enable);
static void SatCs1(bool enable);
static void SatCs2(bool enable);
static void SatCs3(bool enable);
static void SatCs4(bool enable);
static void SatInitPeriph(void);

void SatInit(void)
{
    vsp_dev_t vsp_dev;

    vsp_dev.cb_latch_out    = SatLatchOut;
    vsp_dev.cb_latch_in     = SatLatchIn;
    vsp_dev.cb_cs1          = SatCs1;
    vsp_dev.cb_cs2          = SatCs2;
    vsp_dev.cb_cs3          = SatCs3;
    vsp_dev.cb_cs4          = SatCs4;

    SatInitPeriph();
    VspInit(vsp_dev);
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

    GPIO_PinRemapConfig(GPIO_Remap_SWJ_NoJTRST, ENABLE);

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
    SatLatchOut(false);
    /* CLK */
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = SAT_CLK_PIN;
    GPIO_Init(SAT_CLK_PORT, &GPIO_InitStructure);
    /* MISO */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
    GPIO_InitStructure.GPIO_Pin = SAT_MISO_PIN;
    GPIO_Init(SAT_MISO_PORT, &GPIO_InitStructure);
    /* MISO */
    /* GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
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

    SatCs1(true);
    SatCs2(true);
    SatCs3(true);
    SatCs4(true);

    // SPI_Init(SPI2, &SPI_InitStructure);
    // SPI_Cmd(SPI2, ENABLE);

    NVIC_Init(&NVIC_InitStructure);
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
        GPIO_SetBits(SAT_CS1_PORT, SAT_CS1_PIN);
    else
        GPIO_ResetBits(SAT_CS1_PORT, SAT_CS1_PIN);
}

static void SatCs2(bool enable)
{
    if (enable)
        GPIO_SetBits(SAT_CS2_PORT, SAT_CS2_PIN);
    else
        GPIO_ResetBits(SAT_CS2_PORT, SAT_CS2_PIN);
}

static void SatCs3(bool enable)
{
    if (enable)
        GPIO_SetBits(SAT_CS3_PORT, SAT_CS3_PIN);
    else
        GPIO_ResetBits(SAT_CS3_PORT, SAT_CS3_PIN);
}

static void SatCs4(bool enable)
{
    if (enable)
        GPIO_SetBits(SAT_CS4_PORT, SAT_CS4_PIN);
    else
        GPIO_ResetBits(SAT_CS4_PORT, SAT_CS4_PIN);
}