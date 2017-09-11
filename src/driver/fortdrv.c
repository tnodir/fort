/* Fort Firewall Driver */

#define NDIS_WDM	1
#define NDIS630		1

#include <wdm.h>
#include <fwpmk.h>
#include <fwpsk.h>
#include <stddef.h>
#include <ntrxdef.h>

#include "../common/common.h"
#include "fortdrv.h"

#define FORT_DEVICE_POOL_TAG	'IPFD'

#include "../common/fortconf.c"
#include "../common/fortprov.c"
#include "fortbuf.c"

typedef struct fort_conf_ref {
  UINT32 volatile refcount;

  FORT_CONF conf;
} FORT_CONF_REF, *PFORT_CONF_REF;

typedef struct fort_device {
  BOOL active		: 1;
  BOOL filter_enabled	: 1;
  BOOL prov_temporary	: 1;
  BOOL prov_boot	: 1;

  UINT32 connect4_id;
  UINT32 accept4_id;

  FORT_BUFFER buffer;

  PFORT_CONF_REF volatile conf_ref;
  KSPIN_LOCK conf_lock;
} FORT_DEVICE, *PFORT_DEVICE;

static PFORT_DEVICE g_device = NULL;


static PFORT_CONF_REF
fort_conf_ref_new (const PFORT_CONF conf, ULONG len)
{
  const ULONG ref_len = len + offsetof(FORT_CONF_REF, conf);
  PFORT_CONF_REF conf_ref = ExAllocatePoolWithTag(NonPagedPool, ref_len,
   FORT_DEVICE_POOL_TAG);

  if (conf_ref != NULL) {
    conf_ref->refcount = 0;

    RtlCopyMemory(&conf_ref->conf, conf, len);
  }

  return conf_ref;
}

static void
fort_conf_ref_put (PFORT_CONF_REF conf_ref)
{
  KIRQL irq;

  KeAcquireSpinLock(&g_device->conf_lock, &irq);
  {
    const UINT32 refcount = --conf_ref->refcount;

    if (refcount == 0 && conf_ref != g_device->conf_ref) {
      ExFreePoolWithTag(conf_ref, FORT_DEVICE_POOL_TAG);
    }
  }
  KeReleaseSpinLock(&g_device->conf_lock, irq);
}

static PFORT_CONF_REF
fort_conf_ref_take (void)
{
  PFORT_CONF_REF conf_ref;
  KIRQL irq;

  KeAcquireSpinLock(&g_device->conf_lock, &irq);
  {
    conf_ref = g_device->conf_ref;
    if (conf_ref) {
      ++conf_ref->refcount;
    }
  }
  KeReleaseSpinLock(&g_device->conf_lock, irq);

  return conf_ref;
}

static void
fort_conf_ref_set (PFORT_CONF_REF conf_ref)
{
  PFORT_CONF_REF old_conf_ref;
  KIRQL irq;

  old_conf_ref = fort_conf_ref_take();
  if (old_conf_ref == NULL && conf_ref == NULL)
    return;

  KeAcquireSpinLock(&g_device->conf_lock, &irq);
  {
    g_device->conf_ref = conf_ref;

    g_device->filter_enabled = (conf_ref != NULL
        && conf_ref->conf.flags.filter_enabled);
  }
  KeReleaseSpinLock(&g_device->conf_lock, irq);

  if (old_conf_ref != NULL)
    fort_conf_ref_put(old_conf_ref);
}

static void
fort_conf_ref_flags_set (const PFORT_CONF_FLAGS conf_flags)
{
  KIRQL irq;

  KeAcquireSpinLock(&g_device->conf_lock, &irq);
  {
    PFORT_CONF_REF conf_ref = g_device->conf_ref;

    if (conf_ref) {
      PFORT_CONF conf = &conf_ref->conf;

      g_device->filter_enabled = conf_flags->filter_enabled;

      conf->flags = *conf_flags;

      fort_conf_app_perms_mask_init(conf);
    }
  }
  KeReleaseSpinLock(&g_device->conf_lock, irq);
}

static void
fort_callout_classify_v4 (const FWPS_INCOMING_VALUES0 *inFixedValues,
                          const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues,
                          void *layerData,
                          const FWPS_FILTER0 *filter,
                          UINT64 flowContext,
                          FWPS_CLASSIFY_OUT0 *classifyOut,
                          int flagsField, int localIpField, int remoteIpField)
{
  PFORT_CONF_REF conf_ref;
  UINT32 flags;
  UINT32 remote_ip;
  UINT32 path_len;
  PVOID path;
  BOOL blocked, notify;

  UNUSED(layerData);
  UNUSED(flowContext);

  if (!g_device->filter_enabled) {
    return;
  }

  conf_ref = fort_conf_ref_take();

  if (conf_ref == NULL) {
    if (g_device->prov_boot)
      goto block;
    return;
  }

  flags = inFixedValues->incomingValue[flagsField].value.uint32;
  remote_ip = inFixedValues->incomingValue[remoteIpField].value.uint32;
  path_len = inMetaValues->processPath->size - sizeof(WCHAR);
  path = inMetaValues->processPath->data;

  if (!(flags & FWP_CONDITION_FLAG_IS_LOOPBACK)
      && fort_conf_ip_included(&conf_ref->conf, remote_ip)) {
    blocked = fort_conf_app_blocked(&conf_ref->conf, path_len, path, &notify);
  } else {
    blocked = FALSE;
    notify = FALSE;
  }

  fort_conf_ref_put(conf_ref);

  if (!blocked) {
    classifyOut->actionType = FWP_ACTION_CONTINUE;
    if ((filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT)) {
      classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
    }
    return;
  }

  if (notify) {
    PIRP irp = NULL;
    NTSTATUS irp_status;
    ULONG_PTR info;

    fort_buffer_write(&g_device->buffer, remote_ip,
      (UINT32) inMetaValues->processId, path_len, path,
      &irp, &irp_status, &info);

    if (irp != NULL) {
      fort_request_complete_info(irp, irp_status, info);
    }
  }

 block:
  classifyOut->actionType = FWP_ACTION_BLOCK;
  classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
}

static void
fort_callout_connect_v4 (const FWPS_INCOMING_VALUES0 *inFixedValues,
                         const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues,
                         void *layerData,
                         const FWPS_FILTER0 *filter,
                         UINT64 flowContext,
                         FWPS_CLASSIFY_OUT0 *classifyOut)
{
  fort_callout_classify_v4(inFixedValues, inMetaValues, layerData,
      filter, flowContext, classifyOut,
      FWPS_FIELD_ALE_AUTH_CONNECT_V4_FLAGS,
      FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_LOCAL_ADDRESS,
      FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_REMOTE_ADDRESS);
}

static void
fort_callout_accept_v4 (const FWPS_INCOMING_VALUES0 *inFixedValues,
                        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues,
                        void *layerData,
                        const FWPS_FILTER0 *filter,
                        UINT64 flowContext,
                        FWPS_CLASSIFY_OUT0 *classifyOut)
{
  fort_callout_classify_v4(inFixedValues, inMetaValues, layerData,
      filter, flowContext, classifyOut,
      FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_FLAGS,
      FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_LOCAL_ADDRESS,
      FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_REMOTE_ADDRESS);
}

static NTSTATUS NTAPI
fort_callout_notify (FWPS_CALLOUT_NOTIFY_TYPE notifyType, const GUID *filterKey, const FWPS_FILTER0 *filter)
{
  UNUSED(notifyType);
  UNUSED(filterKey);
  UNUSED(filter);

  return STATUS_SUCCESS;
}

static NTSTATUS
fort_callout_install (PDEVICE_OBJECT device)
{
  FWPS_CALLOUT0 c;
  NTSTATUS status;

  if (g_device->active)
    return STATUS_SHARING_VIOLATION;  // Only one client may connect

  RtlZeroMemory(&c, sizeof(FWPS_CALLOUT0));

  c.notifyFn = fort_callout_notify;

  /* IPv4 connect filter */

  c.calloutKey = FORT_GUID_CALLOUT_CONNECT_V4;
  c.classifyFn = fort_callout_connect_v4;

  status = FwpsCalloutRegister0(device, &c, &g_device->connect4_id);
  if (!NT_SUCCESS(status)) {
    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: Register Connect V4: Error: %d\n", status);
    return status;
  }

  /* IPv4 accept filter */

  c.calloutKey = FORT_GUID_CALLOUT_ACCEPT_V4;
  c.classifyFn = fort_callout_accept_v4;

  status = FwpsCalloutRegister0(device, &c, &g_device->accept4_id);
  if (!NT_SUCCESS(status)) {
    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: Register Accept V4: Error: %d\n", status);
    return status;
  }

  g_device->active = TRUE;

  return STATUS_SUCCESS;
}

static void
fort_callout_remove (void)
{
  if (g_device->connect4_id) {
    FwpsCalloutUnregisterById0(g_device->connect4_id);
  }

  if (g_device->accept4_id) {
    FwpsCalloutUnregisterById0(g_device->accept4_id);
  }

  g_device->active = FALSE;
}

static NTSTATUS
fort_callout_force_reauth (PDEVICE_OBJECT device)
{
  NTSTATUS status;

  // Unregister
  fort_callout_remove();
  fort_prov_unregister();

  // Register
  status = fort_prov_register(!g_device->prov_temporary,
                              g_device->prov_boot, NULL, NULL);

  if (status == STATUS_SUCCESS) {
    status = fort_callout_install(device);
  }

  return status;
}

static NTSTATUS
fort_device_create (PDEVICE_OBJECT device, PIRP irp)
{
  NTSTATUS status;

  status = fort_callout_install(device);

  fort_request_complete(irp, status);

  return status;
}

static NTSTATUS
fort_device_close (PDEVICE_OBJECT device, PIRP irp)
{
  UNUSED(device);

  fort_request_complete(irp, STATUS_SUCCESS);

  return STATUS_SUCCESS;
}

static NTSTATUS
fort_device_cleanup (PDEVICE_OBJECT device, PIRP irp)
{
  UNUSED(device);

  fort_callout_remove();

  fort_conf_ref_set(NULL);

  fort_request_complete(irp, STATUS_SUCCESS);

  return STATUS_SUCCESS;
}

static void
fort_device_cancel_pending (PDEVICE_OBJECT device, PIRP irp)
{
  UNUSED(device);

  fort_buffer_cancel_pending(&g_device->buffer, irp);

  IoReleaseCancelSpinLock(irp->CancelIrql);  /* before IoCompleteRequest()! */

  fort_request_complete(irp, STATUS_CANCELLED);
}

static NTSTATUS
fort_device_control (PDEVICE_OBJECT device, PIRP irp)
{
  PIO_STACK_LOCATION irp_stack;
  ULONG_PTR info = 0;
  NTSTATUS status = STATUS_INVALID_PARAMETER;

  UNUSED(device);

  irp_stack = IoGetCurrentIrpStackLocation(irp);

  switch (irp_stack->Parameters.DeviceIoControl.IoControlCode) {
  case FORT_IOCTL_SETCONF: {
    const PFORT_CONF conf = irp->AssociatedIrp.SystemBuffer;
    const ULONG len = irp_stack->Parameters.DeviceIoControl.InputBufferLength;

    if (conf->flags.conf_version == FORT_CONF_VERSION
        && len > FORT_CONF_DATA_OFF) {
      PFORT_CONF_REF conf_ref = fort_conf_ref_new(conf, len);

      if (conf_ref == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
      } else {
        fort_conf_ref_set(conf_ref);
        status = fort_callout_force_reauth(device);
      }
    }
    break;
  }
  case FORT_IOCTL_SETFLAGS: {
    const PFORT_CONF_FLAGS conf_flags = irp->AssociatedIrp.SystemBuffer;
    const ULONG len = irp_stack->Parameters.DeviceIoControl.InputBufferLength;

    if (conf_flags->conf_version == FORT_CONF_VERSION
        && len == sizeof(FORT_CONF_FLAGS)) {
      fort_conf_ref_flags_set(conf_flags);
      status = fort_callout_force_reauth(device);
    }
    break;
  }
  case FORT_IOCTL_GETLOG: {
    PVOID out = irp->AssociatedIrp.SystemBuffer;
    const ULONG out_len = irp_stack->Parameters.DeviceIoControl.OutputBufferLength;

    status = fort_buffer_xmove(&g_device->buffer, irp, out, out_len, &info);

    if (status == STATUS_PENDING) {
      KIRQL cirq;

      IoMarkIrpPending(irp);

      IoAcquireCancelSpinLock(&cirq);
      IoSetCancelRoutine(irp, fort_device_cancel_pending);
      IoReleaseCancelSpinLock(cirq);

      return STATUS_PENDING;
    }
    break;
  }
  default: break;
  }

  if (!NT_SUCCESS(status)) {
    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: Device Control: Error: %d\n", status);
  }

  fort_request_complete_info(irp, status, info);

  return status;
}

static void
fort_driver_unload (PDRIVER_OBJECT driver)
{
  UNICODE_STRING device_link;

  fort_buffer_close(&g_device->buffer);

  if (g_device->prov_temporary)
    fort_prov_unregister();

  RtlInitUnicodeString(&device_link, DOS_DEVICE_NAME);
  IoDeleteSymbolicLink(&device_link);

  IoDeleteDevice(driver->DeviceObject);
}

NTSTATUS
DriverEntry (PDRIVER_OBJECT driver, PUNICODE_STRING reg_path)
{
  UNICODE_STRING device_name;
  PDEVICE_OBJECT device;
  NTSTATUS status;

  UNUSED(reg_path);

  RtlInitUnicodeString(&device_name, NT_DEVICE_NAME);
  status = IoCreateDevice(driver, sizeof(FORT_DEVICE), &device_name,
                          FORT_DEVICE_TYPE, 0, FALSE, &device);

  if (NT_SUCCESS(status)) {
    UNICODE_STRING device_link;

    RtlInitUnicodeString(&device_link, DOS_DEVICE_NAME);
    status = IoCreateSymbolicLink(&device_link, &device_name);

    if (NT_SUCCESS(status)) {
      driver->MajorFunction[IRP_MJ_CREATE] = fort_device_create;
      driver->MajorFunction[IRP_MJ_CLOSE] = fort_device_close;
      driver->MajorFunction[IRP_MJ_CLEANUP] = fort_device_cleanup;
      driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = fort_device_control;
      driver->DriverUnload = fort_driver_unload;

      device->Flags |= DO_BUFFERED_IO;

      g_device = device->DeviceExtension;

      RtlZeroMemory(g_device, sizeof(FORT_DEVICE));

      fort_buffer_init(&g_device->buffer);

      KeInitializeSpinLock(&g_device->conf_lock);

      // Register filters provider
      {
        BOOL is_temp = FALSE, is_boot = FALSE;

        status = fort_prov_register(FALSE, FALSE, &is_temp, &is_boot);

        g_device->prov_temporary = is_temp;
        g_device->prov_boot = is_boot;
      }
    }
  }

  if (!NT_SUCCESS(status)) {
    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: Entry: Error: %d\n", status);
    fort_driver_unload(driver);
  }

  return status;
}
