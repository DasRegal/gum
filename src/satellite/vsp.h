#ifndef _VSP_H
#define _VSP_H

#include <stdbool.h>
#include <stdlib.h>

#define VSP_MAX_ITEMS       28

typedef struct
{
    bool is_enable;
} vsp_item_t;

typedef struct
{
    void (*cb_latch_out)(bool enable);
    void (*cb_latch_in)(bool enable);
    void (*cb_cs1)(bool enable);
    void (*cb_cs2)(bool enable);
    void (*cb_cs3)(bool enable);
    void (*cb_cs4)(bool enable);
    vsp_item_t item[VSP_MAX_ITEMS];
} vsp_dev_t;

void VspInit(vsp_dev_t dev_struct);

#endif /* _VSP_H */