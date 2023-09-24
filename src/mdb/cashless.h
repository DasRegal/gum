#ifndef _CASHLESS_H
#define _CASHLESS_H

#include <stdbool.h>

void CashlessInit(void);
void CashlessEnable(void);
void CashlessDisable(void);
void CashlessEnableForceCmd(bool enable);
void CashlessDisableForceCmd(bool disable);
void CashlessShowState(uint8_t *state1, uint8_t *state2);
void CashlessResetCmd(void);
uint8_t CashlessVendRequest(uint16_t price, uint16_t item);
bool CashlessIsVendApproved(uint16_t *item);
void CashlessVendSuccessCmd(uint16_t item);
void CashlessVendFailureCmd(void);
uint8_t CashlessVendCancelCmd(void);

#endif /* _CASHLESS_H */