#ifndef _FLOW_H
#define _FLOW_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define FLOW_ITEMS_MAX      28

typedef struct
{
    uint16_t button;
    uint32_t price;
} flow_item_t;

typedef enum
{
    FLOW_STATE_DISABLE,
    FLOW_STATE_IDLE,
    FLOW_STATE_VEND,
    FLOW_STATE_CASHLESS_EN,
    FLOW_STATE_ERROR,
} flow_state_t;

typedef struct
{
    flow_state_t    state;
    bool            temp_val;
    uint32_t        balance;
    flow_item_t     (*items)[FLOW_ITEMS_MAX];
} flow_dev_t;

void FlowInit(void);
flow_dev_t *FlowGetDev(void);
void FlowChangeTempVal(bool val);
bool FlowGetTempVal(void);
flow_state_t FlowStateGet(void);
bool FlowCoinBoxIsEnable(void);
uint32_t FlowGetBalance(void);
void FlowCycle(void);

#endif /* _FLOW_H */
