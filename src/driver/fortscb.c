/* Fort Firewall Driver System Callbacks */

#include "fortscb.h"

#include "fortcb.h"
#include "fortdbg.h"
#include "fortdev.h"

static NTSTATUS fort_syscb_register(
        PCWSTR sourcePath, PCALLBACK_OBJECT *cb_obj, PVOID *cb_reg, PCALLBACK_FUNCTION cb_func)
{
    NTSTATUS status;

    UNICODE_STRING obj_name;
    RtlInitUnicodeString(&obj_name, sourcePath);

    OBJECT_ATTRIBUTES obj_attr;
    InitializeObjectAttributes(&obj_attr, &obj_name, OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = ExCreateCallback(cb_obj, &obj_attr, FALSE, TRUE);

    if (NT_SUCCESS(status)) {
        *cb_reg = ExRegisterCallback(*cb_obj, cb_func, NULL);
    }

    return status;
}

static void fort_syscb_unregister(PCALLBACK_OBJECT cb_obj, PVOID cb_reg)
{
    if (cb_reg != NULL) {
        ExUnregisterCallback(cb_reg);
    }

    if (cb_obj != NULL) {
        ObDereferenceObject(cb_obj);
    }
}

static void NTAPI fort_syscb_power(PVOID context, PVOID event, PVOID specifics)
{
    UNUSED(context);

    FORT_CHECK_STACK(FORT_SYSCB_POWER);

    if (event != (PVOID) PO_CB_SYSTEM_STATE_LOCK)
        return;

    const BOOL power_off = (specifics == NULL);

    fort_device_flag_set(&fort_device()->conf, FORT_DEVICE_POWER_OFF, power_off);

    if (power_off) {
        fort_shaper_drop_packets(&fort_device()->shaper);
    }
}

FORT_API NTSTATUS fort_syscb_power_register(void)
{
    return fort_syscb_register(L"\\Callback\\PowerState", &fort_device()->power_cb_obj,
            &fort_device()->power_cb_reg,
            FORT_CALLBACK(FORT_CALLBACK_SYSCB_POWER, PCALLBACK_FUNCTION, &fort_syscb_power));
}

FORT_API void fort_syscb_power_unregister(void)
{
    fort_syscb_unregister(fort_device()->power_cb_obj, fort_device()->power_cb_reg);
}

static void NTAPI fort_syscb_time(PVOID context, PVOID event, PVOID specifics)
{
    UNUSED(context);
    UNUSED(event);
    UNUSED(specifics);

    FORT_CHECK_STACK(FORT_SYSCB_TIME);

    fort_stat_flags_set(&fort_device()->stat, FORT_STAT_SYSTEM_TIME_CHANGED, TRUE);

    fort_device_on_system_time();
}

FORT_API NTSTATUS fort_syscb_time_register(void)
{
    return fort_syscb_register(L"\\Callback\\SetSystemTime", &fort_device()->systime_cb_obj,
            &fort_device()->systime_cb_reg,
            FORT_CALLBACK(FORT_CALLBACK_SYSCB_TIME, PCALLBACK_FUNCTION, &fort_syscb_time));
}

FORT_API void fort_syscb_time_unregister(void)
{
    fort_syscb_unregister(fort_device()->systime_cb_obj, fort_device()->systime_cb_reg);
}
