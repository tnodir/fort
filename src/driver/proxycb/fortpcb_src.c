/* Fort Firewall Driver Loader: Proxy Callbacks: Source */

#include "fortpcb_src.h"

static ProxyCallbackProc g_proxySrcCallbacks[PROXY_CALLBACKS_COUNT] = {
    proxyCallback0,
    proxyCallback1,
    proxyCallback2,
    proxyCallback3,
    proxyCallback4,
};

FORT_API void fort_proxycb_src_setup(void)
{
    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: ProxyCbSrc Setup: %p\n",
            &proxyCallback0);
}
