#include "src/cctalk/cctalk.h"
#include "flow.h"
#include "src/lcd/dwin.h"
#include "src/mdb/cashless.h"
#include "src/lcd/lcd.h"
#include "satellite.h"

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
static void FlowCashlessEnFunc(void);
static bool FlowGetPriceForItem(uint8_t button, uint32_t *price);
static void FlowVendRequest(void);
static void FlowVendItem(void);
static bool FlowIsTimeout(void);

void FlowInit(void)
{
    flow_dev.state      = FLOW_STATE_IDLE;
    flow_dev.temp_val   = false;
    flow_dev.balance    = 0;
    flow_dev.items      = flow_items;
    flow_dev.is_timeout = false;
}

bool FlowCycle(flow_state_t *state)
{
    flow_state_t cur_state = flow_dev.state;


    switch(flow_dev.state)
    {
        case FLOW_STATE_DISABLE:
            break;
        case FLOW_STATE_IDLE:
            FlowIdleFunc();
            break;
        case FLOW_STATE_VEND:
            FlowVendItem();
            break;
        case FLOW_STATE_CASHLESS_EN:
            FlowCashlessEnFunc();
            break;
        case FLOW_STATE_VEND_REQUEST:
            FlowVendRequest();
            break;
        default:
            break;
    }

    if (flow_dev.state != cur_state)
    {
        *state = flow_dev.state;
        return true;
    }

    return false;
}

static bool FlowIsTimeout(void)
{
    if (!flow_dev.is_timeout)
    {
        return false;
    }

    flow_dev.is_timeout = false;
    return true;
}

void FlowVendTimeout(void)
{
    flow_dev.is_timeout = true;
}

void FlowInitCycle(void)
{
    LcdUpdateBalance(flow_dev.balance);
    DwinSetPage(1);
}

static void FlowIdleFunc(void)
{
    uint16_t    button;
    uint32_t    price;
    bool        status = false;

    status = DwinIsPushButton(&button);
    if (!status)
        return;

    DwinHandleButton(button);

    status = FlowGetPriceForItem(button & 0xff, &price);
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

static void FlowCashlessEnFunc(void)
{
    uint32_t price;
    uint8_t  item;

    item = flow_dev.item;
    price = flow_dev.items[item].price - flow_dev.balance;
    DwinSetPage(2);
    SatPushLcdButton(flow_dev.items[item].button);
    CashlessVendRequest(price, item);
    flow_dev.state = FLOW_STATE_VEND_REQUEST;

    return;
}

static void FlowVendRequest(void)
{
    if (CashlessIsVendApproved(NULL))
    {
        DwinSetPage(3);
        SatVend(flow_dev.item);
        flow_dev.state = FLOW_STATE_VEND;
        return;
    }

    if (FlowIsTimeout())
    {
        DwinSetPage(1);
        flow_dev.state = FLOW_STATE_IDLE;
    }

}

static void FlowVendItem(void)
{
    uint8_t res = 1;
    
    res = SatIsVendOk();
    
    if (FlowIsTimeout())
    {
        res = 2;
    }

    switch(res)
    {
        case 1:
            flow_dev.balance = 0;
            CashlessVendSuccessCmd(0);
            LcdUpdateBalance(flow_dev.balance);
            DwinSetPage(1);
            flow_dev.state = FLOW_STATE_IDLE;
            break;
        case 2:
            CashlessVendFailureCmd();
            LcdUpdateBalance(flow_dev.balance);
            DwinSetPage(1);
            flow_dev.state = FLOW_STATE_IDLE;
            break;
        case 0:
        default:
            break;
    }
}

static bool FlowGetPriceForItem(uint8_t button, uint32_t *price)
{
    for (uint8_t i = 0; i < FLOW_ITEMS_MAX; i++)
    {
        if (flow_dev.items[i].button == button)
        {
            *price = flow_dev.items[i].price;
            flow_dev.item = i;
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
