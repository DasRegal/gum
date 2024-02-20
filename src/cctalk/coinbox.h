#ifndef _COINBOX_H
#define _COINBOX_H

#include "cctalk.h"
#include <stdio.h>

void CoinBoxInit(void);
void UsartSendString_Cctalk(const char *pucBuffer);
void CoinBoxCliCmdSendData(uint8_t hdr, uint8_t *buf, uint8_t len);
bool CoinBoxIsUpdateBalance(uint32_t *balance);

#endif /* _COINBOX_H */
