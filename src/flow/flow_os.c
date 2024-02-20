#include "flow_os.h"
#include "flow.h"
#include "satellite.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

TimerHandle_t xFlowItemSelTimer;
static TimerHandle_t xBalanceUpdateTimer;

void vTaskFlowCycle(void *pvParameters);
void vTimerFlowItemSelTimeoutCb( TimerHandle_t xTimer );
void vTimerBalanceUpdateCb( TimerHandle_t xTimer );

void FlowOsInit(void)
{
    FlowInit();
    xFlowItemSelTimer = xTimerCreate ( "FlowSelTimeout", 30000, pdFALSE, ( void * ) 0, vTimerFlowItemSelTimeoutCb );
    // xBalanceUpdateTimer = xTimerCreate ( "Bal Update", 50, pdFALSE, ( void * ) 0, vTimerBalanceUpdateCb );
    xTimerStop(xFlowItemSelTimer, 0);
    xTaskCreate(vTaskFlowCycle, "FlowCycle", 1024, NULL, tskIDLE_PRIORITY + 2, NULL);
}

void vTaskFlowCycle(void *pvParameters)
{
    flow_state_t state;

    FlowInitCycle();
    // xTimerStart(xBalanceUpdateTimer, 0);
    for ( ;; )
    {
        if (FlowCycle(&state))
        {
            if (state == FLOW_STATE_VEND)
            {
                xTimerStart(xFlowItemSelTimer, 0);
            }

            if (state == FLOW_STATE_VEND_REQUEST)
            {
                xTimerStart(xFlowItemSelTimer, 0);
            }
        }
        vTaskDelay(100);
    }
}

void vTimerFlowItemSelTimeoutCb( TimerHandle_t xTimer )
{
    FlowVendTimeout();
    SatVendTimeout();
    xTimerStop(xFlowItemSelTimer, 0);
}

// void vTimerBalanceUpdateCb( TimerHandle_t xTimer )
// {
//     FlowBalanceUpdateCb();
// }