/* Fort Firewall Driver Loader: Proxy Callbacks: Dummy */

#include "fortpcb_def.h"

#define ProxyCallbackFunction(i)                                                                   \
    void proxyCallback##i(void) { g_proxyDstProcs[i](); }

ProxyCallbackProc g_proxyDstProcs[PROXY_CALLBACKS_COUNT];

ProxyCallbackFunction(0) ProxyCallbackFunction(1) ProxyCallbackFunction(2) ProxyCallbackFunction(3)
        ProxyCallbackFunction(4)
