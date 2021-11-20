#ifndef FORTPCB_DST_H
#define FORTPCB_DST_H

#include "fortpcb_def.h"

#if defined(__cplusplus)
extern "C" {
#endif

extern ProxyCallbackProc g_proxyDstCallbacks[PROXY_CALLBACKS_COUNT];

FORT_API void fort_proxycb_dst_setup(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTPCB_DST_H
