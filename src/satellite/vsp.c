#include "vsp.h"
vsp_dev_t vsp_dev;

static void VspLatchOut(bool enable);
static void VspLatchIn(bool enable);
static void VspCs1(bool enable);
static void VspCs2(bool enable);
static void VspCs3(bool enable);
static void VspCs4(bool enable);

void VspInit(vsp_dev_t dev_struct)
{
    vsp_dev.cb_latch_out = dev_struct.cb_latch_out;
    vsp_dev.cb_latch_in  = dev_struct.cb_latch_in;
    vsp_dev.cb_cs1       = dev_struct.cb_cs1;
    vsp_dev.cb_cs2       = dev_struct.cb_cs2;
    vsp_dev.cb_cs3       = dev_struct.cb_cs3;
    vsp_dev.cb_cs4       = dev_struct.cb_cs4;

    for (size_t i = 0; i < VSP_MAX_ITEMS; i++)
        vsp_dev.item[i].is_enable = false;
}

static void VspLatchOut(bool enable)
{
    if (vsp_dev.cb_latch_out == NULL)
        return;

    vsp_dev.cb_latch_out(enable);
}

static void VspLatchIn(bool enable)
{
    if (vsp_dev.cb_latch_in == NULL)
        return;

    vsp_dev.cb_latch_in(enable);
}

static void VspCs1(bool enable)
{
    if (vsp_dev.cb_cs1 == NULL)
        return;

    vsp_dev.cb_cs1(enable);
}

static void VspCs2(bool enable)
{
    if (vsp_dev.cb_cs2 == NULL)
        return;

    vsp_dev.cb_cs2(enable);
}

static void VspCs3(bool enable)
{
    if (vsp_dev.cb_cs3 == NULL)
        return;

    vsp_dev.cb_cs3(enable);
}

static void VspCs4(bool enable)
{
    if (vsp_dev.cb_cs4 == NULL)
        return;

    vsp_dev.cb_cs4(enable);
}