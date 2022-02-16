/* Fort Firewall Driver Loader: Proxy Callbacks: Source */

#include "fortpcb_src.h"

ProxyCallbackProc g_proxyDstCallbacksArray[PROXY_CALLBACKS_COUNT];

#define ProxyCallbackExtern(i) extern void WINAPI proxyDstCallback##i(void)

ProxyCallbackExtern(0);
ProxyCallbackExtern(1);
ProxyCallbackExtern(2);
ProxyCallbackExtern(3);
ProxyCallbackExtern(4);
ProxyCallbackExtern(5);
ProxyCallbackExtern(6);
ProxyCallbackExtern(7);
ProxyCallbackExtern(8);
ProxyCallbackExtern(9);
ProxyCallbackExtern(10);
ProxyCallbackExtern(11);
ProxyCallbackExtern(12);
ProxyCallbackExtern(13);
ProxyCallbackExtern(14);
ProxyCallbackExtern(15);
ProxyCallbackExtern(16);
ProxyCallbackExtern(17);
ProxyCallbackExtern(18);
ProxyCallbackExtern(19);
ProxyCallbackExtern(20);
ProxyCallbackExtern(21);
ProxyCallbackExtern(22);
ProxyCallbackExtern(23);
ProxyCallbackExtern(24);
ProxyCallbackExtern(25);
ProxyCallbackExtern(26);
ProxyCallbackExtern(27);
ProxyCallbackExtern(28);
ProxyCallbackExtern(29);
ProxyCallbackExtern(30);
ProxyCallbackExtern(31);
ProxyCallbackExtern(32);
ProxyCallbackExtern(33);
ProxyCallbackExtern(34);
ProxyCallbackExtern(35);
ProxyCallbackExtern(36);
ProxyCallbackExtern(37);
ProxyCallbackExtern(38);
ProxyCallbackExtern(39);
ProxyCallbackExtern(40);
ProxyCallbackExtern(41);
ProxyCallbackExtern(42);
ProxyCallbackExtern(43);
ProxyCallbackExtern(44);
ProxyCallbackExtern(45);
ProxyCallbackExtern(46);
ProxyCallbackExtern(47);
ProxyCallbackExtern(48);
ProxyCallbackExtern(49);
ProxyCallbackExtern(50);
ProxyCallbackExtern(51);
ProxyCallbackExtern(52);
ProxyCallbackExtern(53);
ProxyCallbackExtern(54);
ProxyCallbackExtern(55);
ProxyCallbackExtern(56);
ProxyCallbackExtern(57);
ProxyCallbackExtern(58);
ProxyCallbackExtern(59);
ProxyCallbackExtern(60);
ProxyCallbackExtern(61);
ProxyCallbackExtern(62);
ProxyCallbackExtern(63);

static ProxyCallbackProc g_proxySrcCallbacks[PROXY_CALLBACKS_COUNT] = {
    proxyDstCallback0,
    proxyDstCallback1,
    proxyDstCallback2,
    proxyDstCallback3,
    proxyDstCallback4,
    proxyDstCallback5,
    proxyDstCallback6,
    proxyDstCallback7,
    proxyDstCallback8,
    proxyDstCallback9,
    proxyDstCallback10,
    proxyDstCallback11,
    proxyDstCallback12,
    proxyDstCallback13,
    proxyDstCallback14,
    proxyDstCallback15,
    proxyDstCallback16,
    proxyDstCallback17,
    proxyDstCallback18,
    proxyDstCallback19,
    proxyDstCallback20,
    proxyDstCallback21,
    proxyDstCallback22,
    proxyDstCallback23,
    proxyDstCallback24,
    proxyDstCallback25,
    proxyDstCallback26,
    proxyDstCallback27,
    proxyDstCallback28,
    proxyDstCallback29,
    proxyDstCallback30,
    proxyDstCallback31,
    proxyDstCallback32,
    proxyDstCallback33,
    proxyDstCallback34,
    proxyDstCallback35,
    proxyDstCallback36,
    proxyDstCallback37,
    proxyDstCallback38,
    proxyDstCallback39,
    proxyDstCallback40,
    proxyDstCallback41,
    proxyDstCallback42,
    proxyDstCallback43,
    proxyDstCallback44,
    proxyDstCallback45,
    proxyDstCallback46,
    proxyDstCallback47,
    proxyDstCallback48,
    proxyDstCallback49,
    proxyDstCallback50,
    proxyDstCallback51,
    proxyDstCallback52,
    proxyDstCallback53,
    proxyDstCallback54,
    proxyDstCallback55,
    proxyDstCallback56,
    proxyDstCallback57,
    proxyDstCallback58,
    proxyDstCallback59,
    proxyDstCallback60,
    proxyDstCallback61,
    proxyDstCallback62,
    proxyDstCallback63,
};

FORT_API void fort_proxycb_src_prepare(PFORT_PROXYCB_INFO cbInfo)
{
    cbInfo->src = g_proxySrcCallbacks;
    cbInfo->dst = g_proxyDstCallbacksArray;
}

FORT_API void fort_proxycb_src_setup(PFORT_PROXYCB_INFO cbInfo)
{
#if defined(_WIN64) && !defined(_M_ARM64)
    memcpy(g_proxyDstCallbacksArray, cbInfo->dst, sizeof(g_proxyDstCallbacksArray));
#else
    UNUSED(cbInfo);
#endif
}
