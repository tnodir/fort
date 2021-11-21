#ifndef FORTPCB_DST_H
#define FORTPCB_DST_H

#include "fortpcb_def.h"

#if defined(__cplusplus)
extern "C" {
#endif

extern ProxyCallbackProc *g_proxyCallbacksPtr;

FORT_API void fort_proxycb_dst_prepare(PFORT_PROXYCB_INFO cbInfo);
FORT_API void fort_proxycb_dst_setup(PFORT_PROXYCB_INFO cbInfo);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTPCB_DST_H
