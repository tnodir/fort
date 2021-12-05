/* Fort Firewall Driver Loader: Proxy Callbacks: Driver Major Functions */

#include "fortpcb_drv.h"

#define FORT_DRIVER_MAJOR_FUNC_MAX (IRP_MJ_MAXIMUM_FUNCTION + 1)

static_assert(FORT_DRIVER_MAJOR_FUNC_MAX == 28, "Driver Major Functions Count Mismatch");

static PDRIVER_DISPATCH g_proxyDrvCallbacks[FORT_DRIVER_MAJOR_FUNC_MAX];

#define ProxyMajorProc(i)                                                                          \
    static NTSTATUS proxyMajor##i(PDEVICE_OBJECT device, PIRP irp)                                 \
    {                                                                                              \
        return g_proxyDrvCallbacks[(i)]((device), (irp));                                          \
    }

ProxyMajorProc(0);
ProxyMajorProc(1);
ProxyMajorProc(2);
ProxyMajorProc(3);
ProxyMajorProc(4);
ProxyMajorProc(5);
ProxyMajorProc(6);
ProxyMajorProc(7);
ProxyMajorProc(8);
ProxyMajorProc(9);
ProxyMajorProc(10);
ProxyMajorProc(11);
ProxyMajorProc(12);
ProxyMajorProc(13);
ProxyMajorProc(14);
ProxyMajorProc(15);
ProxyMajorProc(16);
ProxyMajorProc(17);
ProxyMajorProc(18);
ProxyMajorProc(19);
ProxyMajorProc(20);
ProxyMajorProc(21);
ProxyMajorProc(22);
ProxyMajorProc(23);
ProxyMajorProc(24);
ProxyMajorProc(25);
ProxyMajorProc(26);
ProxyMajorProc(27);

static PDRIVER_DISPATCH g_proxyMajorProcs[FORT_DRIVER_MAJOR_FUNC_MAX] = {
    proxyMajor0,
    proxyMajor1,
    proxyMajor2,
    proxyMajor3,
    proxyMajor4,
    proxyMajor5,
    proxyMajor6,
    proxyMajor7,
    proxyMajor8,
    proxyMajor9,
    proxyMajor10,
    proxyMajor11,
    proxyMajor12,
    proxyMajor13,
    proxyMajor14,
    proxyMajor15,
    proxyMajor16,
    proxyMajor17,
    proxyMajor18,
    proxyMajor19,
    proxyMajor20,
    proxyMajor21,
    proxyMajor22,
    proxyMajor23,
    proxyMajor24,
    proxyMajor25,
    proxyMajor26,
    proxyMajor27,
};

FORT_API void fort_proxycb_drv_setup(PDRIVER_DISPATCH *driver_major_funcs)
{
    for (int i = 0; i < FORT_DRIVER_MAJOR_FUNC_MAX; ++i) {
        PDRIVER_DISPATCH drv_func = driver_major_funcs[i];
        if (drv_func != NULL) {
            g_proxyDrvCallbacks[i] = drv_func;
            driver_major_funcs[i] = g_proxyMajorProcs[i];

            DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
                    "FORT: fort_proxycb_drv_setup: %d mf=%p\n", i, drv_func);
        }
    }
}
