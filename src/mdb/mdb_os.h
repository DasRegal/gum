#ifndef _MDB_OS_H
#define _MDB_OS_H

void MdbOsInit(void);
void CashlessEnable(void);
void CashlessDisable(void);
uint8_t CashlessMakePurchase(uint16_t item, uint16_t price);

#endif /* _MDB_OS_H */
