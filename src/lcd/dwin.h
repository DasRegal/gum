#ifndef _DWIN_H
#define _DWIN_H

void DwinInit(void);
void DwinGetCharHandler(uint8_t ch);
bool DwinIsPushButton(uint16_t *button);
void DwinHandleButton(uint16_t button);

#endif /* _DWIN_H */
