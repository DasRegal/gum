#ifndef _DWIN_H
#define _DWIN_H

#include <stdbool.h>
void DwinInit(void (*cb_send)(const char*, uint8_t));
bool DwinGetCharHandler(uint8_t ch);
bool DwinIsPushButton(uint16_t *button);
void DwinHandleButton(uint16_t button);
void DwinSetPage(uint8_t page);
void DwinReset(void);
void DwinButtonEn(uint16_t button, bool isEnable);

#endif /* _DWIN_H */
