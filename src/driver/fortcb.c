/* Fort Firewall Driver Callbacks */

#include "fortcb.h"

#include "proxycb/fortpcb_dst.h"

FORT_API FortCallbackFunc fort_callback(int id, FortCallbackFunc func)
{
    g_proxyDstCallbacks[id] = func;
    return func;
}
