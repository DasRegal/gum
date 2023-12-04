#include <stdio.h>

#include "stm32f10x.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

#include "main.h"
#include "src/hw_rev_adc/hw_rev_adc.h"
#include "src/hw_voltage_adc/hw_voltage_adc.h"
#include "src/hw_adc.h"
#include "src/mdb/cashless.h"
#include "src/cctalk/coinbox.h"
#include "src/lcd/lcd.h"
#include "src/satellite/satellite.h"

SemaphoreHandle_t semTickPulse;
static void PrintBanner(void);

void vTaskCode ( void *pvParameters)
{
    for( ;; )
    {
        vTaskDelay(1000);
    }
}

void main()
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

    ConsoleInit();
    PrintBanner();
    PRINT_OS("\r\n");

    HwAdcInit();
    HwRevPrint();
    HwVoltPrint();
    LcdInit();

    CashlessInit();

    ConsoleCliStart();

    CoinBoxInit();
    SatInit();

    xTaskCreate(
        vTaskCode,
        "Task",
        256,
        NULL,
        tskIDLE_PRIORITY + 1,
        NULL
    );

    vTaskStartScheduler();
    while(1);
}

static void PrintBanner(void)
{
    PRINT_OS("\r\n");
    PRINT_OS("    ____ _    __               __\r\n");
    PRINT_OS("   / __ \\ |  / /__  ____  ____/ /\r\n");
    PRINT_OS("  / /_/ / | / / _ \\/ __ \\/ __  /\r\n");
    PRINT_OS(" / _, _/| |/ /  __/ / / / /_/ /\r\n");
    PRINT_OS("/_/ |_| |___/\\___/_/ /_/\\__,_/\r\n");
    PRINT_OS("                               RVend\r\n");
}

// =============================================================================
//
//         Задача демон, выполняющийся при запуске планировщика FreeRTOS
//
// =============================================================================

void vApplicationDaemonTaskStartupHook( void )
{

}

void vApplicationMallocFailedHook( void )
{

    for( ;; );
}
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{

    for( ;; );
}
void vApplicationIdleHook( void )
{
}
