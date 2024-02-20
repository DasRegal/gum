#ifndef _SATELLITE_H
#define _SATELLITE_H

#include <stdint.h>

void SatInit(void);
void SatContinuePoll(void);
uint8_t SatGetPushButton(void);
// void SatResume(void);
void SatVend(uint8_t item);
uint8_t SatIsVendOk(void);
void SatPushLcdButton(uint8_t button);
void SatVendTimeout();
bool SatIsPushButton(uint16_t *button);

#endif /* _SATELLITE_H */
