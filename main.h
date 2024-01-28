#ifndef _MAIN_H
#define _MAIN_H

#include <stdio.h>
#include "src/console/console.h"

#define PRINT_OS(s)		UsartDebugSendString(s)

#ifndef GIT_HASH
#define GIT_HASH "0"
#endif

#endif /* _MAIN_H */