/* Fort Firewall Driver Loader: Proxy Callbacks: Destination */

#include "fortpcb_dst.h"

ProxyCallbackProc g_proxyCallbacksArray[PROXY_CALLBACKS_COUNT];

#if defined(_WIN64) && !defined(_M_ARM64)

#    define ProxyCallbackExtern(i) extern void proxyCallback##i(void)

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

static ProxyCallbackProc g_proxyDstCallbacks[PROXY_CALLBACKS_COUNT] = {
    proxyCallback0,
    proxyCallback1,
    proxyCallback2,
    proxyCallback3,
    proxyCallback4,
    proxyCallback5,
    proxyCallback6,
    proxyCallback7,
    proxyCallback8,
    proxyCallback9,
    proxyCallback10,
    proxyCallback11,
    proxyCallback12,
    proxyCallback13,
    proxyCallback14,
    proxyCallback15,
    proxyCallback16,
    proxyCallback17,
    proxyCallback18,
    proxyCallback19,
    proxyCallback20,
    proxyCallback21,
    proxyCallback22,
    proxyCallback23,
    proxyCallback24,
    proxyCallback25,
    proxyCallback26,
    proxyCallback27,
    proxyCallback28,
    proxyCallback29,
    proxyCallback30,
    proxyCallback31,
    proxyCallback32,
    proxyCallback33,
    proxyCallback34,
    proxyCallback35,
    proxyCallback36,
    proxyCallback37,
    proxyCallback38,
    proxyCallback39,
    proxyCallback40,
    proxyCallback41,
    proxyCallback42,
    proxyCallback43,
    proxyCallback44,
    proxyCallback45,
    proxyCallback46,
    proxyCallback47,
    proxyCallback48,
    proxyCallback49,
    proxyCallback50,
    proxyCallback51,
    proxyCallback52,
    proxyCallback53,
    proxyCallback54,
    proxyCallback55,
    proxyCallback56,
    proxyCallback57,
    proxyCallback58,
    proxyCallback59,
    proxyCallback60,
    proxyCallback61,
    proxyCallback62,
    proxyCallback63,
};

#endif

FORT_API void fort_proxycb_dst_setup(PFORT_PROXYCB_INFO cbInfo)
{
#if defined(_WIN64) && !defined(_M_ARM64)
    cbInfo->dst = g_proxyDstCallbacks;
    cbInfo->callbacks = g_proxyCallbacksArray;
#else
    cbInfo->callbacks = cbInfo->dst;
#endif
}
