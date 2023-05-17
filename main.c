#include "stm32f10x.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

#include "main.h"
#include "src/console/console.h"

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

    ConsoleInit();
    PRINT_OS("\r\n");
    PRINT_OS("Version HW 1.0\r\n");

    ConsoleCliStart();

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
