/* Fort Firewall Driver Device */

#include "fortdev.h"

#include "common/fortioctl.h"
#include "common/fortprov.h"

#include "fortcout.h"
#include "fortdbg.h"
#include "fortscb.h"
#include "forttrace.h"
#include "fortutl.h"

static PFORT_DEVICE g_device = NULL;

typedef struct fort_device_control_arg
{
    PIRP irp;
    ULONG_PTR *info;

    PVOID buffer;
    ULONG in_len;
    ULONG out_len;
} FORT_DEVICE_CONTROL_ARG, *PFORT_DEVICE_CONTROL_ARG;

FORT_API PFORT_DEVICE fort_device(void)
{
    return g_device;
}

FORT_API void fort_device_set(PFORT_DEVICE device)
{
    g_device = device;
}

static NTSTATUS fort_device_reauth_force(const FORT_CONF_FLAGS old_conf_flags)
{
    PEX_RUNDOWN_REF reauth_rundown = &fort_device()->reauth_rundown;
    ExAcquireRundownProtection(reauth_rundown);

    const NTSTATUS status = fort_callout_force_reauth(old_conf_flags);

    ExReleaseRundownProtection(reauth_rundown);

    return status;
}

static void fort_device_reauth(void)
{
    const FORT_CONF_FLAGS conf_flags = fort_device()->conf.conf_flags;

    fort_device_reauth_force(conf_flags);
}

static void fort_device_reauth_queue(void)
{
    fort_worker_queue(&fort_device()->worker, FORT_WORKER_REAUTH);
}

static void fort_app_period_timer(void)
{
    if (fort_conf_ref_period_update(&fort_device()->conf, /*force=*/FALSE, /*periods_n=*/NULL)) {
        fort_device_reauth_queue();
    }
}

FORT_API void fort_device_on_system_time(void)
{
    if (fort_timer_is_running(&fort_device()->app_timer)) {
        fort_app_period_timer();
    }
}

FORT_API NTSTATUS fort_device_create(PDEVICE_OBJECT device, PIRP irp)
{
    UNUSED(device);

    FORT_CHECK_STACK(FORT_DEVICE_CREATE);

    NTSTATUS status = STATUS_SUCCESS;

    /* Device opened */
    const UCHAR flags = fort_device_flag_set(&fort_device()->conf, FORT_DEVICE_IS_OPENED, TRUE);
    if ((flags & FORT_DEVICE_IS_OPENED) != 0) {
        status = STATUS_SHARING_VIOLATION; /* Only one client may connect */
    }

    if (NT_SUCCESS(status)) {
        /* Clear buffer */
        fort_buffer_clear(&fort_device()->buffer);
    }

    fort_request_complete(irp, status);

    return status;
}

FORT_API NTSTATUS fort_device_close(PDEVICE_OBJECT device, PIRP irp)
{
    UNUSED(device);

    FORT_CHECK_STACK(FORT_DEVICE_CLOSE);

    fort_request_complete(irp, STATUS_SUCCESS);

    return STATUS_SUCCESS;
}

FORT_API NTSTATUS fort_device_cleanup(PDEVICE_OBJECT device, PIRP irp)
{
    UNUSED(device);

    FORT_CHECK_STACK(FORT_DEVICE_CLEANUP);

    /* Device closed */
    fort_device_flag_set(
            &fort_device()->conf, (FORT_DEVICE_IS_OPENED | FORT_DEVICE_IS_VALIDATED), FALSE);

    /* Clear conf */
    {
        const FORT_CONF_FLAGS old_conf_flags = fort_conf_ref_set(&fort_device()->conf, NULL);
        FORT_CONF_FLAGS conf_flags = fort_device()->conf.conf_flags;

        fort_conf_zones_set(&fort_device()->conf, NULL);

        fort_stat_conf_flags_update(&fort_device()->stat, &conf_flags);

        fort_device_reauth_force(old_conf_flags);
    }

    /* Clear pending packets */
    fort_pending_clear(&fort_device()->pending);

    /* Clear buffer */
    fort_buffer_clear(&fort_device()->buffer);

    fort_request_complete(irp, STATUS_SUCCESS);

    return STATUS_SUCCESS;
}

static NTSTATUS fort_device_control_validate(PFORT_DEVICE_CONTROL_ARG dca)
{
    const PFORT_CONF_VERSION conf_ver = dca->buffer;
    const ULONG len = dca->in_len;

    if (len == sizeof(FORT_CONF_VERSION)) {
        if (conf_ver->driver_version == DRIVER_VERSION) {
            fort_device_flag_set(&fort_device()->conf, FORT_DEVICE_IS_VALIDATED, TRUE);
            return STATUS_SUCCESS;
        }
    }

    return STATUS_UNSUCCESSFUL;
}

static NTSTATUS fort_device_control_setservices(PFORT_DEVICE_CONTROL_ARG dca)
{
    const PFORT_SERVICE_INFO_LIST services = dca->buffer;
    const ULONG len = dca->in_len;

    if (len > sizeof(FORT_SERVICE_INFO_LIST)) {
        fort_pstree_update_services(&fort_device()->ps_tree, services,
                /*data_len=*/len - FORT_SERVICE_INFO_LIST_DATA_OFF);

        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

inline static NTSTATUS fort_device_control_setconf_ref(
        const PFORT_CONF_IO conf_io, PFORT_CONF_REF conf_ref)
{
    PFORT_DEVICE_CONF device_conf = &fort_device()->conf;
    const BOOL was_null_conf = (device_conf->ref == NULL);

    const FORT_CONF_FLAGS old_conf_flags = fort_conf_ref_set(device_conf, conf_ref);

    fort_stat_conf_update(&fort_device()->stat, conf_io);
    fort_shaper_conf_update(&fort_device()->shaper, conf_io);

    /* Enumerate processes */
    if (was_null_conf) {
        fort_pstree_enum_processes(&fort_device()->ps_tree);
    }

    return fort_device_reauth_force(old_conf_flags);
}

static NTSTATUS fort_device_control_setconf(PFORT_DEVICE_CONTROL_ARG dca)
{
    const PFORT_CONF_IO conf_io = dca->buffer;
    const ULONG len = dca->in_len;

    if (len > sizeof(FORT_CONF_IO)) {
        const PFORT_CONF conf = &conf_io->conf;
        PFORT_CONF_REF conf_ref = fort_conf_ref_new(conf, len - FORT_CONF_IO_CONF_OFF);

        if (conf_ref == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        return fort_device_control_setconf_ref(conf_io, conf_ref);
    }

    return STATUS_UNSUCCESSFUL;
}

static NTSTATUS fort_device_control_setflags(PFORT_DEVICE_CONTROL_ARG dca)
{
    const PFORT_CONF_FLAGS conf_flags = dca->buffer;
    const ULONG len = dca->in_len;

    if (len == sizeof(FORT_CONF_FLAGS)) {
        const FORT_CONF_FLAGS old_conf_flags =
                fort_conf_ref_flags_set(&fort_device()->conf, conf_flags);

        fort_stat_conf_flags_update(&fort_device()->stat, conf_flags);
        fort_shaper_conf_flags_update(&fort_device()->shaper, conf_flags);

        return fort_device_reauth_force(old_conf_flags);
    }

    return STATUS_UNSUCCESSFUL;
}

static NTSTATUS fort_device_control_getlog(PFORT_DEVICE_CONTROL_ARG dca)
{
    PVOID out = dca->buffer;
    const ULONG out_len = dca->out_len;

    if (out_len < FORT_BUFFER_SIZE)
        return STATUS_BUFFER_TOO_SMALL;

    PIRP irp = dca->irp;
    ULONG_PTR *info = dca->info;

    const NTSTATUS status = fort_buffer_xmove(&fort_device()->buffer, irp, out, out_len, info);

    if (status == STATUS_PENDING) {
        fort_buffer_irp_mark_pending(irp);
    }

    return status;
}

inline static NTSTATUS fort_device_control_app_conf(
        const PFORT_APP_ENTRY app_entry, PFORT_CONF_REF conf_ref, BOOL is_adding)
{
    NTSTATUS status;

    if (is_adding) {
        status = fort_conf_ref_exe_add_entry(conf_ref, app_entry, /*locked=*/FALSE);
    } else {
        fort_conf_ref_exe_del_entry(conf_ref, app_entry);
        status = STATUS_SUCCESS;
    }

    return status;
}

static NTSTATUS fort_device_control_app(PFORT_DEVICE_CONTROL_ARG dca, BOOL is_adding)
{
    const PFORT_APP_ENTRY app_entry = dca->buffer;
    const ULONG len = dca->in_len;

    if (len < sizeof(FORT_APP_ENTRY) || len < FORT_CONF_APP_ENTRY_SIZE(app_entry->path_len))
        return STATUS_UNSUCCESSFUL;

    PFORT_CONF_REF conf_ref = fort_conf_ref_take(&fort_device()->conf);

    if (conf_ref == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    const NTSTATUS status = fort_device_control_app_conf(app_entry, conf_ref, is_adding);

    fort_conf_ref_put(&fort_device()->conf, conf_ref);

    if (NT_SUCCESS(status)) {
        fort_device_reauth_queue();
    }

    return status;
}

static NTSTATUS fort_device_control_addapp(PFORT_DEVICE_CONTROL_ARG dca)
{
    return fort_device_control_app(dca, /*is_adding=*/TRUE);
}

static NTSTATUS fort_device_control_delapp(PFORT_DEVICE_CONTROL_ARG dca)
{
    return fort_device_control_app(dca, /*is_adding=*/FALSE);
}

static NTSTATUS fort_device_control_setzones(PFORT_DEVICE_CONTROL_ARG dca)
{
    const PFORT_CONF_ZONES zones = dca->buffer;
    const ULONG len = dca->in_len;

    if (len >= FORT_CONF_ZONES_DATA_OFF) {
        PFORT_CONF_ZONES conf_zones = fort_conf_zones_new(zones, len);

        if (conf_zones == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        } else {
            fort_conf_zones_set(&fort_device()->conf, conf_zones);

            fort_device_reauth_queue();

            return STATUS_SUCCESS;
        }
    }

    return STATUS_UNSUCCESSFUL;
}

static NTSTATUS fort_device_control_setzoneflag(PFORT_DEVICE_CONTROL_ARG dca)
{
    const PFORT_CONF_ZONE_FLAG zone_flag = dca->buffer;
    const ULONG len = dca->in_len;

    if (len == sizeof(FORT_CONF_ZONE_FLAG)) {
        fort_conf_zone_flag_set(&fort_device()->conf, zone_flag);

        fort_device_reauth_queue();

        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

static_assert(FORT_CTL_INDEX_FROM_CODE(FORT_IOCTL_SETZONEFLAG) == FORT_IOCTL_INDEX_SETZONEFLAG,
        "Invalid FORT_CTL_INDEX_FROM_CODE()");

typedef NTSTATUS(FORT_DEVICE_CONTROL_PROCESS_FUNC)(PFORT_DEVICE_CONTROL_ARG dca);
typedef FORT_DEVICE_CONTROL_PROCESS_FUNC *PFORT_DEVICE_CONTROL_PROCESS_FUNC;

static PFORT_DEVICE_CONTROL_PROCESS_FUNC fortDeviceControlProcess_funcList[] = {
    &fort_device_control_validate,
    &fort_device_control_setservices,
    &fort_device_control_setconf,
    &fort_device_control_setflags,
    &fort_device_control_getlog,
    &fort_device_control_addapp,
    &fort_device_control_delapp,
    &fort_device_control_setzones,
    &fort_device_control_setzoneflag,
};

static NTSTATUS fort_device_control_process(
        const PIO_STACK_LOCATION irp_stack, PIRP irp, ULONG_PTR *info)
{
    const UCHAR control_index =
            FORT_CTL_INDEX_FROM_CODE(irp_stack->Parameters.DeviceIoControl.IoControlCode);

    if (control_index > FORT_IOCTL_INDEX_SETZONEFLAG)
        return STATUS_INVALID_PARAMETER;

    if (control_index != FORT_IOCTL_INDEX_VALIDATE
            && fort_device_flag(&fort_device()->conf, FORT_DEVICE_IS_VALIDATED) == 0)
        return STATUS_INVALID_DEVICE_REQUEST;

    FORT_DEVICE_CONTROL_ARG dca = {
        .irp = irp,
        .info = info,

        .buffer = irp->AssociatedIrp.SystemBuffer,
        .in_len = irp_stack->Parameters.DeviceIoControl.InputBufferLength,
        .out_len = irp_stack->Parameters.DeviceIoControl.OutputBufferLength,
    };

    PFORT_DEVICE_CONTROL_PROCESS_FUNC func = fortDeviceControlProcess_funcList[control_index];

    return func(&dca);
}

FORT_API NTSTATUS fort_device_control(PDEVICE_OBJECT device, PIRP irp)
{
    UNUSED(device);

    FORT_CHECK_STACK(FORT_DEVICE_CONTROL);

    ULONG_PTR info = 0;

    const PIO_STACK_LOCATION irp_stack = IoGetCurrentIrpStackLocation(irp);
    const NTSTATUS status = fort_device_control_process(irp_stack, irp, &info);

    if (!NT_SUCCESS(status) && status != FORT_STATUS_USER_ERROR) {
        LOG("Device Control: Error: %x\n", status);
        TRACE(FORT_DEVICE_DEVICE_CONTROL_ERROR, status, 0, 0);
    }

    if (status != STATUS_PENDING) {
        fort_request_complete_info(irp, status, info);
    }

    return status;
}

FORT_API NTSTATUS fort_device_shutdown(PDEVICE_OBJECT device, PIRP irp)
{
    UNUSED(device);

    FORT_CHECK_STACK(FORT_DEVICE_SHUTDOWN);

    if (fort_device() != NULL) {
        fort_stat_close_flows(&fort_device()->stat);
    }

    fort_request_complete(irp, STATUS_SUCCESS);

    return STATUS_SUCCESS;
}

static NTSTATUS fort_device_register_provider(void)
{
    NTSTATUS status;

    fort_prov_init();

    HANDLE engine;
    status = fort_prov_trans_open(&engine);
    if (!NT_SUCCESS(status))
        return status;

    FORT_PROV_BOOT_CONF boot_conf = { .v = 0 };

    if (!fort_prov_get_boot_conf(engine, &boot_conf)) {
        // Default flags from Registry
        boot_conf.boot_filter = fort_reg_flag(L"BootFilter");
    }

    fort_device_flag_set(&fort_device()->conf, FORT_DEVICE_BOOT_FILTER, boot_conf.boot_filter);
    fort_device_flag_set(
            &fort_device()->conf, FORT_DEVICE_BOOT_FILTER_LOCALS, boot_conf.filter_locals);

    fort_prov_unregister(engine);

    status = fort_prov_register(engine, boot_conf);

    return fort_prov_trans_close(engine, status);
}

FORT_API NTSTATUS fort_device_load(PVOID device_param)
{
    FORT_CHECK_STACK(FORT_DEVICE_LOAD);

    NTSTATUS status;

    PDEVICE_OBJECT device = device_param;

    ExInitializeRundownProtection(&fort_device()->reauth_rundown);

    fort_worker_func_set(&fort_device()->worker, FORT_WORKER_REAUTH, &fort_device_reauth);

    fort_device_conf_open(&fort_device()->conf);
    fort_buffer_open(&fort_device()->buffer);
    fort_stat_open(&fort_device()->stat);
    fort_pending_open(&fort_device()->pending);
    fort_shaper_open(&fort_device()->shaper);
    fort_timer_open(&fort_device()->log_timer, 500, /*flags=*/0, &fort_callout_timer);
    fort_timer_open(
            &fort_device()->app_timer, 60000, FORT_TIMER_COALESCABLE, &fort_app_period_timer);
    fort_pstree_open(&fort_device()->ps_tree);

    /* Register filters provider */
    status = fort_device_register_provider();
    if (!NT_SUCCESS(status))
        return status;

    /* Install callouts */
    status = fort_callout_install(device);
    if (!NT_SUCCESS(status))
        return status;

    /* Register worker */
    status = fort_worker_register(device, &fort_device()->worker);
    if (!NT_SUCCESS(status))
        return status;

    /* Register power state change callback */
    status = fort_syscb_power_register();
    if (!NT_SUCCESS(status))
        return status;

    /* Register system time change callback */
    status = fort_syscb_time_register();
    if (!NT_SUCCESS(status))
        return status;

    return STATUS_SUCCESS;
}

FORT_API void fort_device_unload(void)
{
    FORT_CHECK_STACK(FORT_DEVICE_UNLOAD);

    /* Stop system notifiers */
    fort_syscb_power_unregister();
    fort_syscb_time_unregister();

    /* Stop timers */
    fort_timer_close(&fort_device()->app_timer);
    fort_timer_close(&fort_device()->log_timer);

    /* Stop worker threads */
    fort_worker_unregister(&fort_device()->worker);

    /* Stop process monitor */
    fort_pstree_close(&fort_device()->ps_tree);

    /* Stop packets shaper & pending */
    fort_shaper_close(&fort_device()->shaper);
    fort_pending_close(&fort_device()->pending);

    /* Stop stat & buffer controllers */
    fort_stat_close(&fort_device()->stat);
    fort_buffer_close(&fort_device()->buffer);

    /* Uninstall callouts */
    fort_callout_remove();

    /* Unregister filters provider */
    if (fort_device_flag(&fort_device()->conf, FORT_DEVICE_BOOT_FILTER) == 0) {
        fort_prov_trans_unregister();
    }
}
