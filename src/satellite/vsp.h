#ifndef _VSP_H
#define _VSP_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

#define VSP_MAX_ITEMS       4

typedef struct
{
    bool inhibit;
    bool led;
    bool motor;
} vsp_item_state_t;

typedef struct
{
    bool is_enable;
    void (*cb_cs)(bool enable);
    bool block;
    bool is_select;
    vsp_item_state_t state;
} vsp_item_t;

typedef struct
{
    void (*cb_latch_out)(bool enable);
    void (*cb_latch_in)(bool enable);
    void (*cb_clk)(bool enable);
    bool (*cb_sat_sens)(void);
    bool (*cb_sat_button)(void);
    vsp_item_t item[VSP_MAX_ITEMS];
} vsp_dev_t;

void VspInit(vsp_dev_t dev_struct);
void VspSelectItem(uint8_t idx);
void VspDeselectItem(uint8_t idx);
uint8_t VspGetSelectItem(void);
void VspLedCtrl(uint8_t idx, bool enable);
void VspMotorCtrl(uint8_t idx, bool enable);
bool VspButton(uint8_t idx);
bool VspCheck(uint8_t idx);
void VspInhibitCtrl(uint8_t idx, bool enable);
void VspEnable(uint8_t idx, bool enable);
void VspBlock(uint8_t idx, bool enable);

#endif /* _VSP_H */