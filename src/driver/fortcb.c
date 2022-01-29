/* Fort Firewall Driver Callbacks */

#include "fortcb.h"

#include "proxycb/fortpcb_dst.h"

FORT_PROXYCB_INFO g_callbackInfo;

FORT_API FortCallbackFunc fort_callback(int id, FortCallbackFunc func)
{
    if (g_callbackInfo.src == NULL)
        return func;

    ProxyCallbackProc cb = g_callbackInfo.src[id];

#ifdef FORT_DEBUG
    LOG("Proxy Callback: i=%d func=%p cb=%p\n", id, func, cb);
#endif

    g_callbackInfo.callbacks[id] = func;
    return cb;
}

FORT_API void fort_callback_setup(PFORT_PROXYCB_INFO cb_info)
{
    fort_proxycb_dst_setup(cb_info);

    g_callbackInfo = *cb_info;
}
