#include "src/cctalk/cctalk.h"
#include "flow.h"
#include "src/lcd/dwin.h"

flow_dev_t flow_dev;

flow_item_t flow_items[FLOW_ITEMS_MAX] =
{
    { 0,  120 },
    { 1,  50  },
    { 2,  1   },
    { 3,  40  },
    { 4,  0 },
    { 5,  0 },
    { 6,  0 },
    { 7,  0 },
    { 8,  0 },
    { 9,  0 },
    { 10, 0 },
    { 11, 0 },
    { 12, 0 },
    { 13, 0 },
    { 14, 0 },
    { 15, 0 },
    { 16, 0 },
    { 17, 0 },
    { 18, 0 },
    { 19, 0 },
    { 20, 0 },
    { 21, 0 },
    { 22, 0 },
    { 23, 0 },
    { 24, 0 },
    { 25, 0 },
    { 26, 0 },
    { 27, 0 },
};

static void FlowIdleFunc(void);
static bool FlowGetPriceForItem(uint8_t button, uint32_t *price);

void FlowInit(void)
{
    flow_dev.state      = FLOW_STATE_DISABLE;
    flow_dev.temp_val   = false;
    flow_dev.balance    = 0;
    flow_dev.items      = &flow_items;
}

void FlowCycle(void)
{
    switch(flow_dev.state)
    {
        case FLOW_STATE_DISABLE:
            break;
        case FLOW_STATE_IDLE:
            FlowIdleFunc();
            break;
        case FLOW_STATE_VEND:
            break;
        default:
            break;
    }
}

static void FlowIdleFunc(void)
{
    uint16_t    button;
    uint32_t    price;
    bool        status = false;

    status = DwinIsPushButton(&button);
    if (!status)
        return;

    status = FlowGetPriceForItem(button, &price);
    if (!status)
        return;

    if (flow_dev.balance >= price)
    {
        flow_dev.state = FLOW_STATE_VEND;
        return;
    }

    flow_dev.state = FLOW_STATE_CASHLESS_EN;
    return;
}

static bool FlowGetPriceForItem(uint8_t button, uint32_t *price)
{
    for (uint8_t i = 0; i < FLOW_ITEMS_MAX; i++)
    {
        if (flow_dev.items[i]->button == button)
        {
            *price = flow_dev.items[i]->price;
            return true;
        }
    }

    return false;
}

void FlowEnable(void)
{
    flow_dev.state = FLOW_STATE_IDLE;
}

flow_dev_t *FlowGetDev(void)
{
    return &flow_dev;
}

flow_state_t FlowStateGet(void)
{
    return flow_dev.state;
}

bool FlowCoinBoxIsEnable(void)
{
    bool isEnable = false;

    isEnable = CctalkIsEnable();

    return isEnable;
}

uint32_t FlowGetBalance(void)
{
    return flow_dev.balance;
}

void FlowSetBalance(uint32_t balance)
{
    flow_dev.balance = balance;
}

void FlowAddToBalance(uint32_t balance)
{
    flow_dev.balance += balance;
}

void FlowChangeTempVal(bool val)
{
    flow_dev.temp_val = val;
}

bool FlowUpdateBalance()
{
    uint32_t balance = 0;
    bool isUpdate = false;
    /* Coin get balance */
    /* Cashless get balance */
    /* Bill get balance */
    if (false /**/)
    {
        flow_dev.balance += balance;
        isUpdate = true;
    }

    return isUpdate;
}

bool FlowGetTempVal(void)
{
    return flow_dev.temp_val;
}
