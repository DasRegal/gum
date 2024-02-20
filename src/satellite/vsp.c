#include "vsp.h"
vsp_dev_t vsp_dev;

static void VspCs(uint8_t idx, bool enable);

void VspInit(vsp_dev_t dev_struct)
{
    vsp_dev.cb_latch_out    = dev_struct.cb_latch_out;
    vsp_dev.cb_latch_in     = dev_struct.cb_latch_in;
    vsp_dev.cb_clk          = dev_struct.cb_clk;
    vsp_dev.item[0].cb_cs   = dev_struct.item[0].cb_cs;
    vsp_dev.item[1].cb_cs   = dev_struct.item[1].cb_cs;
    vsp_dev.item[2].cb_cs   = dev_struct.item[2].cb_cs;
    vsp_dev.item[3].cb_cs   = dev_struct.item[3].cb_cs;
    vsp_dev.cb_sat_sens     = dev_struct.cb_sat_sens;
    vsp_dev.cb_sat_button   = dev_struct.cb_sat_button;

    for (size_t i = 0; i < VSP_MAX_ITEMS; i++)
    {
        vsp_dev.item[i].is_enable   = true;
        vsp_dev.item[i].block       = false;
        vsp_dev.item[i].is_select   = false;
        vsp_dev.item[i].state.inhibit = false;
        vsp_dev.item[i].state.led   = false;
        vsp_dev.item[i].state.motor = false;
    }
}

static void VspCs(uint8_t idx, bool enable)
{
    if (idx >= VSP_MAX_ITEMS)
        return;
    if (vsp_dev.item[idx].cb_cs == NULL)
        return;

    vsp_dev.item[idx].cb_cs(enable);
}

void VspEnable(uint8_t idx, bool enable)
{
    if (idx >= VSP_MAX_ITEMS)
        return;

    vsp_dev.item[idx].is_enable = enable;
}

bool VspIsEnable(uint8_t idx)
{
    if (idx >= VSP_MAX_ITEMS)
        return false;

    return vsp_dev.item[idx].is_enable;
}

void VspBlock(uint8_t idx, bool enable)
{
    if (idx >= VSP_MAX_ITEMS)
        return;

    VspMotorCtrl(idx, false);
    VspLedCtrl(idx, false);
    vsp_dev.item[idx].block = enable;
}

bool VspIsBlock(uint8_t idx)
{
    if (idx >= VSP_MAX_ITEMS)
        return false;

    return vsp_dev.item[idx].block;
}

void VspMotorCtrl(uint8_t idx, bool enable)
{
    if (true == VspIsBlock(idx))
        return;

    if (false == VspIsEnable(idx))
        return;

    vsp_dev.item[idx].state.motor = enable;
    vsp_dev.cb_latch_in(vsp_dev.item[idx].state.motor);
    vsp_dev.cb_latch_out(vsp_dev.item[idx].state.led);
    vsp_dev.cb_clk(vsp_dev.item[idx].state.inhibit);
    VspCs(idx, true);
    VspCs(idx, false);
}

void VspLedCtrl(uint8_t idx, bool enable)
{
    if (true == VspIsBlock(idx))
        return;

    if (false == VspIsEnable(idx))
        return;

    vsp_dev.item[idx].state.led = enable;
    vsp_dev.cb_latch_in(vsp_dev.item[idx].state.motor);
    vsp_dev.cb_latch_out(vsp_dev.item[idx].state.led);
    vsp_dev.cb_clk(vsp_dev.item[idx].state.inhibit);

    VspCs(idx, true);
    VspCs(idx, false);
}

void VspInhibitCtrl(uint8_t idx, bool enable)
{
    if (idx >= VSP_MAX_ITEMS)
        return;

    if (false == VspIsEnable(idx))
        return;

    vsp_dev.item[idx].state.inhibit = enable;
    vsp_dev.cb_latch_in(vsp_dev.item[idx].state.motor);
    vsp_dev.cb_latch_out(vsp_dev.item[idx].state.led);
    vsp_dev.cb_clk(vsp_dev.item[idx].state.inhibit);

    VspCs(idx, true);
}

bool VspCheck(uint8_t idx)
{
    bool ret = false;

    if (VspIsBlock(idx) == true)
        return false;

    vsp_dev.cb_latch_in(vsp_dev.item[idx].state.motor);
    vsp_dev.cb_latch_out(vsp_dev.item[idx].state.led);
    vsp_dev.cb_clk(vsp_dev.item[idx].state.inhibit);

    VspCs(idx, true);
    ret = vsp_dev.cb_sat_sens();
    VspCs(idx, false); 

    VspEnable(idx, ret);

    return ret;  
}

bool VspButton(uint8_t idx)
{
    bool ret = false;
    
    if (true == VspIsBlock(idx))
        return false;

    if (false == VspIsEnable(idx))
        return false;

    vsp_dev.cb_latch_in(vsp_dev.item[idx].state.motor);
    vsp_dev.cb_latch_out(vsp_dev.item[idx].state.led);
    vsp_dev.cb_clk(vsp_dev.item[idx].state.inhibit);

    VspCs(idx, true);
    ret = vsp_dev.cb_sat_button();
    VspCs(idx, false);

    return ret;
}

void VspSelectItem(uint8_t idx)
{
    if (false == VspIsEnable(idx))
        return;

    for (size_t i = 0; i < VSP_MAX_ITEMS; i++)
    {
        VspLedCtrl(i, (i == idx) ? true : false);
    }

    vsp_dev.item[idx].is_select = true;
}

void VspDeselectItem(uint8_t idx)
{
    if (false == VspIsEnable(idx))
        return;

    for (size_t i = 0; i < VSP_MAX_ITEMS; i++)
    {
        VspLedCtrl(i, false);
    }

    vsp_dev.item[idx].is_select = false;
}

uint8_t VspGetSelectItem(void)
{
    for (size_t i = 0; i < VSP_MAX_ITEMS; i++)
    {
        if (vsp_dev.item[i].is_select)
            return i;
    }

    return VSP_MAX_ITEMS;
}
