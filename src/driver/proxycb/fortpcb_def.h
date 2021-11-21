#ifndef FORTPCB_DEF_H
#define FORTPCB_DEF_H

#include "../fortdrv.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef void (*ProxyCallbackProc)(void);

#define PROXY_CALLBACKS_COUNT 64

#define ProxyCallbackExtern(i) extern void proxyCallback##i(void)

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

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTPCB_DEF_H
