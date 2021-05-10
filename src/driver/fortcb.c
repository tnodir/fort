/* Fort Firewall Driver Callbacks */

#include "fortdrv.h"

#include "fortcout.h"
#include "fortdev.h"

static NTSTATUS fort_callback_register(
        PCWSTR sourcePath, PCALLBACK_OBJECT *cb_obj, PVOID *cb_reg, PCALLBACK_FUNCTION cb_func)
{
    OBJECT_ATTRIBUTES obj_attr;
    UNICODE_STRING obj_name;
    NTSTATUS status;

    RtlInitUnicodeString(&obj_name, sourcePath);

    InitializeObjectAttributes(&obj_attr, &obj_name, OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = ExCreateCallback(cb_obj, &obj_attr, FALSE, TRUE);

    if (NT_SUCCESS(status)) {
        *cb_reg = ExRegisterCallback(*cb_obj, cb_func, NULL);
    }

    return status;
}

static void fort_callback_unregister(PCALLBACK_OBJECT cb_obj, PVOID cb_reg)
{
    if (cb_reg != NULL) {
        ExUnregisterCallback(cb_reg);
    }

    if (cb_obj != NULL) {
        ObDereferenceObject(cb_obj);
    }
}

static void fort_callback_power(PVOID context, PVOID event, PVOID specifics)
{
    UNUSED(context);

    if (event != (PVOID) PO_CB_SYSTEM_STATE_LOCK)
        return;

    const BOOL power_off = (specifics == NULL);

    fort_device_flag_set(&fort_device()->conf, FORT_DEVICE_POWER_OFF, power_off);

    if (power_off) {
        fort_callout_defer_flush();
    }
}

FORT_API NTSTATUS fort_callback_power_register(void)
{
    return fort_callback_register(L"\\Callback\\PowerState", &fort_device()->power_cb_obj,
            &fort_device()->power_cb_reg, fort_callback_power);
}

FORT_API void fort_callback_power_unregister(void)
{
    fort_callback_unregister(fort_device()->power_cb_obj, fort_device()->power_cb_reg);
}

static void fort_callback_systime(PVOID context, PVOID event, PVOID specifics)
{
    UNUSED(context);
    UNUSED(event);
    UNUSED(specifics);

    if (fort_device()->app_timer.running) {
        fort_app_period_timer();
    }
}

FORT_API NTSTATUS fort_callback_systime_register(void)
{
    return fort_callback_register(L"\\Callback\\SetSystemTime", &fort_device()->systime_cb_obj,
            &fort_device()->systime_cb_reg, fort_callback_systime);
}

FORT_API void fort_callback_systime_unregister(void)
{
    fort_callback_unregister(fort_device()->systime_cb_obj, fort_device()->systime_cb_reg);
}
