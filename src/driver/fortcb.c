/* Fort Firewall Driver Callbacks */

#include "fortcb.h"

#include "proxycb/fortpcb_dst.h"

FORT_PROXYCB_INFO g_callbackInfo;

FORT_API FortCallbackFunc fort_callback(int id, FortCallbackFunc func)
{
    if (g_callbackInfo.src == NULL)
        return func;

    g_callbackInfo.callbacks[id] = func;
    return g_callbackInfo.src[id];
}

FORT_API void fort_callback_setup(PFORT_PROXYCB_INFO cb_info)
{
    if (cb_info == NULL) {
        g_callbackInfo.src = NULL;
    } else {
        fort_proxycb_dst_setup(cb_info);

        g_callbackInfo = *cb_info;
    }
}
