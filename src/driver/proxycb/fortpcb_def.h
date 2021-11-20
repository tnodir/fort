#ifndef FORTPCB_DEF_H
#define FORTPCB_DEF_H

#include "../fortdrv.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef void (*ProxyCallbackProc)(void);

#define PROXY_CALLBACKS_COUNT 64

#define ProxyCallbackExtern(i) extern void proxyCallback##i(void)

ProxyCallbackExtern(0);
ProxyCallbackExtern(1);
ProxyCallbackExtern(2);
ProxyCallbackExtern(3);
ProxyCallbackExtern(4);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTPCB_DEF_H
