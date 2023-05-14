#include <stdio.h>

#include "stm32f10x.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

#include "src/console/console.h"

SemaphoreHandle_t semTickPulse;

void vTaskCode ( void *pvParameters)
{
    for( ;; )
    {
//        if(xSemaphoreTake(semTickPulse, 1) == pdTRUE)
//        {
//            GPIO_SetBits(GPIOC, GPIO_Pin_8);
//        }
//        else
//        {
//            GPIO_ResetBits(GPIOC, GPIO_Pin_8);
//        }
      vTaskDelay(10000);
    }
}

void main()
{
//    NVIC_SetVectorTable( NVIC_VectTab_FLASH, 0x0 );
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

    // RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    // GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

    ConsoleInit();
    UsartDebugSendString((uint8_t*)"USART OK\r\n");
    vSemaphoreCreateBinary(semTickPulse);
    xSemaphoreTake(semTickPulse, 0);

    xTaskCreate(
        vTaskCode,
        "Task",
        configMINIMAL_STACK_SIZE,
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