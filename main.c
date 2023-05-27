#include <stdio.h>

#include "stm32f10x.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

#include "main.h"
#include "src/console/console.h"
#include "src/hw_rev_adc/hw_rev_adc.h"
#include "src/hw_voltage_adc/hw_voltage_adc.h"
#include "src/hw_adc.h"
#include "src/mdb/mdb_os.h"

SemaphoreHandle_t semTickPulse;

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

    //ConsoleInit();
    //PRINT_OS("\r\n");

    //HwAdcInit();
    //HwRevPrint();
    //HwVoltPrint();

    MdbOsInit();
    // for(int i = 0; i < 50000; i++)
    // {
    //     asm("NOP");
    // }
    //MdbPrint();

    //ConsoleCliStart();

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
