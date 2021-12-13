#ifndef FORTPCB_DEF_H
#define FORTPCB_DEF_H

#include "../fortdrv.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef void(WINAPI *ProxyCallbackProc)(void);

typedef struct fort_proxycb_info
{
    ProxyCallbackProc *src;
    ProxyCallbackProc *dst;
    ProxyCallbackProc *callbacks;
} FORT_PROXYCB_INFO, *PFORT_PROXYCB_INFO;

#define PROXY_CALLBACKS_COUNT 64

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTPCB_DEF_H
