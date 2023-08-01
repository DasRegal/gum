#ifndef _COINBOX_H
#define _COINBOX_H

#include "cctalk.h"

void CoinBoxInit(void);
void UsartSendString_Cctalk(const char *pucBuffer);

#endif /* _COINBOX_H */