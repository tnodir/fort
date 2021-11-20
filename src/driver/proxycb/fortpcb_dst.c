/* Fort Firewall Driver Loader: Proxy Callbacks: Destination */

#include "fortpcb_dst.h"

ProxyCallbackProc g_proxyDstCallbacks[PROXY_CALLBACKS_COUNT];

FORT_API void fort_proxycb_dst_setup(void)
{
    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: ProxyCbDst Setup: %p\n",
            &proxyCallback0);
}
