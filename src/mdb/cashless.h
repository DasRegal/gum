#ifndef _CASHLESS_H
#define _CASHLESS_H

#include <stdbool.h>

void CashlessInit(void);
void CashlessEnable(void);
void CashlessDisable(void);
void CashlessEnableForce(bool enable);
void CashlessDisableForce(bool disable);
void CashlessShowState(uint8_t *state);

#endif /* _CASHLESS_H */