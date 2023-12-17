#ifndef _SATELLITE_H
#define _SATELLITE_H

#include <stdint.h>

void SatInit(void);
void SatContinuePoll(void);
uint8_t SatGetPushButton(void);

#endif /* _SATELLITE_H */