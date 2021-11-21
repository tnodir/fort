/* Fort Firewall Driver Callbacks */

#include "fortcb.h"

#include "proxycb/fortpcb_dst.h"

FORT_PROXYCB_INFO g_callbackInfo;

FORT_API FortCallbackFunc fort_callback(int id, FortCallbackFunc func)
{
    g_callbackInfo.callbacks[id] = func;
    return g_callbackInfo.src[id];
}

void fort_callback_setup(PFORT_PROXYCB_INFO cb_info)
{
    fort_proxycb_dst_prepare(&g_callbackInfo);

    if (cb_info == NULL) {
        g_callbackInfo.src = g_callbackInfo.dst;
    } else {
        g_callbackInfo.src = cb_info->src;

        fort_proxycb_dst_setup(&g_callbackInfo);

        *cb_info = g_callbackInfo;
    }
}
