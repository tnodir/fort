#ifndef FORTPCB_SRC_H
#define FORTPCB_SRC_H

#include "fortpcb_def.h"

#if defined(__cplusplus)
extern "C" {
#endif

extern ProxyCallbackProc g_proxyDstCallbacksArray[PROXY_CALLBACKS_COUNT];

FORT_API void fort_proxycb_src_prepare(PFORT_PROXYCB_INFO cbInfo);
FORT_API void fort_proxycb_src_setup(PFORT_PROXYCB_INFO cbInfo);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTPCB_SRC_H
