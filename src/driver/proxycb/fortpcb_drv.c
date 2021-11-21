/* Fort Firewall Driver Loader: Proxy Callbacks: Driver Major Functions */

#include "fortpcb_drv.h"

#define FORT_DRIVER_MAJOR_FUNC_MAX (IRP_MJ_MAXIMUM_FUNCTION + 1)

static_assert(FORT_DRIVER_MAJOR_FUNC_MAX == 28, "Driver Major Functions Count Mismatch");

static PDRIVER_DISPATCH g_proxyDrvCallbacks[FORT_DRIVER_MAJOR_FUNC_MAX];

#define DispatchProc(i)                                                                            \
    static NTSTATUS dispatch##i(PDEVICE_OBJECT device, PIRP irp)                                   \
    {                                                                                              \
        return g_proxyDrvCallbacks[(i)]((device), (irp));                                          \
    }

DispatchProc(0);
DispatchProc(1);
DispatchProc(2);
DispatchProc(3);
DispatchProc(4);
DispatchProc(5);
DispatchProc(6);
DispatchProc(7);
DispatchProc(8);
DispatchProc(9);
DispatchProc(10);
DispatchProc(11);
DispatchProc(12);
DispatchProc(13);
DispatchProc(14);
DispatchProc(15);
DispatchProc(16);
DispatchProc(17);
DispatchProc(18);
DispatchProc(19);
DispatchProc(20);
DispatchProc(21);
DispatchProc(22);
DispatchProc(23);
DispatchProc(24);
DispatchProc(25);
DispatchProc(26);
DispatchProc(27);

static PDRIVER_DISPATCH g_dispatchProcs[FORT_DRIVER_MAJOR_FUNC_MAX] = {
    dispatch0,
    dispatch1,
    dispatch2,
    dispatch3,
    dispatch4,
    dispatch5,
    dispatch6,
    dispatch7,
    dispatch8,
    dispatch9,
    dispatch10,
    dispatch11,
    dispatch12,
    dispatch13,
    dispatch14,
    dispatch15,
    dispatch16,
    dispatch17,
    dispatch18,
    dispatch19,
    dispatch20,
    dispatch21,
    dispatch22,
    dispatch23,
    dispatch24,
    dispatch25,
    dispatch26,
    dispatch27,
};

FORT_API void fort_proxycb_drv_setup(PDRIVER_DISPATCH *driver_major_funcs)
{
    for (int i = 0; i < FORT_DRIVER_MAJOR_FUNC_MAX; ++i) {
        PDRIVER_DISPATCH drv_func = driver_major_funcs[i];
        if (drv_func != NULL) {
            g_proxyDrvCallbacks[i] = drv_func;
            driver_major_funcs[i] = g_dispatchProcs[i];

            DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
                    "FORT: fort_proxycb_drv_setup: %d\n", i);
        }
    }
}
