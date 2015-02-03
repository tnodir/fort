/* Windows IP Filter Driver */

#define NDIS_WDM	1
#define NDIS630		1

#include <wdm.h>
#include <fwpmk.h>
#include <fwpsk.h>
#include <stddef.h>
#include <ntrxdef.h>

#include "../common.h"
#include "wipfdrv.h"

#define WIPF_DEVICE_POOL_TAG	'IPFD'

#include "../wipfconf.c"
#include "../wipfprov.c"
#include "wipfbuf.c"

typedef struct wipf_conf_ref {
  UINT32 volatile refcount;

  WIPF_CONF conf;
} WIPF_CONF_REF, *PWIPF_CONF_REF;

typedef struct wipf_device {
  BOOL active		: 1;
  BOOL prov_temporary	: 1;

  UINT32 connect4_id;
  UINT32 accept4_id;

  WIPF_BUFFER buffer;

  PWIPF_CONF_REF volatile conf_ref;
  KSPIN_LOCK conf_lock;
} WIPF_DEVICE, *PWIPF_DEVICE;

static PWIPF_DEVICE g_device = NULL;


static PWIPF_CONF_REF
wipf_conf_ref_new (const PWIPF_CONF conf, ULONG len)
{
  const ULONG ref_len = len + offsetof(WIPF_CONF_REF, conf);
  PWIPF_CONF_REF conf_ref = ExAllocatePoolWithTag(NonPagedPool, ref_len,
   WIPF_DEVICE_POOL_TAG);

  if (conf_ref != NULL) {
    conf_ref->refcount = 0;

    RtlCopyMemory(&conf_ref->conf, conf, len);
  }

  return conf_ref;
}

static void
wipf_conf_ref_put (PWIPF_CONF_REF conf_ref)
{
  KIRQL irq;

  KeAcquireSpinLock(&g_device->conf_lock, &irq);
  {
    const UINT32 refcount = --conf_ref->refcount;

    if (refcount == 0 && conf_ref != g_device->conf_ref) {
      ExFreePoolWithTag(conf_ref, WIPF_DEVICE_POOL_TAG);
    }
  }
  KeReleaseSpinLock(&g_device->conf_lock, irq);
}

static PWIPF_CONF_REF
wipf_conf_ref_take (void)
{
  PWIPF_CONF_REF conf_ref;
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
wipf_conf_ref_set (PWIPF_CONF_REF conf_ref)
{
  PWIPF_CONF_REF old_conf_ref;
  KIRQL irq;

  old_conf_ref = wipf_conf_ref_take();
  if (old_conf_ref == NULL && conf_ref == NULL)
    return;

  KeAcquireSpinLock(&g_device->conf_lock, &irq);
  {
    g_device->conf_ref = conf_ref;
  }
  KeReleaseSpinLock(&g_device->conf_lock, irq);

  if (old_conf_ref != NULL)
    wipf_conf_ref_put(old_conf_ref);
}

static void
wipf_callout_classify_v4 (const FWPS_INCOMING_VALUES0 *inFixedValues,
                          const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues,
                          void *layerData,
                          const FWPS_FILTER0 *filter,
                          UINT64 flowContext,
                          FWPS_CLASSIFY_OUT0 *classifyOut,
                          int flagsField, int localIpField, int remoteIpField)
{
  PWIPF_CONF_REF conf_ref;
  UINT32 flags;
  UINT32 local_ip, remote_ip;
  UINT32 path_len;
  PVOID path;
  BOOL blocked, notify;

  UNUSED(layerData);
  UNUSED(flowContext);

  conf_ref = wipf_conf_ref_take();

  if (conf_ref == NULL)
    return;

  flags = inFixedValues->incomingValue[flagsField].value.uint32;
  local_ip = inFixedValues->incomingValue[localIpField].value.uint32;
  remote_ip = inFixedValues->incomingValue[remoteIpField].value.uint32;
  path_len = inMetaValues->processPath->size;
  path = inMetaValues->processPath->data;

  if (!(flags & FWP_CONDITION_FLAG_IS_LOOPBACK)
      && wipf_conf_ip_included(&conf_ref->conf, remote_ip)) {
    blocked = wipf_conf_app_blocked(&conf_ref->conf, path_len, path, &notify);
  } else {
    blocked = FALSE;
    notify = FALSE;
  }

  wipf_conf_ref_put(conf_ref);

  if (blocked) {
    classifyOut->actionType = FWP_ACTION_BLOCK;
    classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;

    if (notify) {
      PIRP irp = NULL;
      NTSTATUS irp_status;
      ULONG_PTR info;

      wipf_buffer_write(&g_device->buffer, remote_ip,
        (UINT32) inMetaValues->processId, path_len, path,
        &irp, &irp_status, &info);

      if (irp != NULL) {
        wipf_request_complete_info(irp, irp_status, info);
      }
    }
  } else {
    classifyOut->actionType = FWP_ACTION_CONTINUE;
    if ((filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT)) {
      classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
    }
  }
}

static void
wipf_callout_connect_v4 (const FWPS_INCOMING_VALUES0 *inFixedValues,
                         const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues,
                         void *layerData,
                         const FWPS_FILTER0 *filter,
                         UINT64 flowContext,
                         FWPS_CLASSIFY_OUT0 *classifyOut)
{
  wipf_callout_classify_v4(inFixedValues, inMetaValues, layerData,
      filter, flowContext, classifyOut,
      FWPS_FIELD_ALE_AUTH_CONNECT_V4_FLAGS,
      FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_LOCAL_ADDRESS,
      FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_REMOTE_ADDRESS);
}

static void
wipf_callout_accept_v4 (const FWPS_INCOMING_VALUES0 *inFixedValues,
                        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues,
                        void *layerData,
                        const FWPS_FILTER0 *filter,
                        UINT64 flowContext,
                        FWPS_CLASSIFY_OUT0 *classifyOut)
{
  wipf_callout_classify_v4(inFixedValues, inMetaValues, layerData,
      filter, flowContext, classifyOut,
      FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_FLAGS,
      FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_LOCAL_ADDRESS,
      FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_REMOTE_ADDRESS);
}

static NTSTATUS NTAPI
wipf_callout_notify (FWPS_CALLOUT_NOTIFY_TYPE notifyType, const GUID *filterKey, const FWPS_FILTER0 *filter)
{
  UNUSED(notifyType);
  UNUSED(filterKey);
  UNUSED(filter);

  return STATUS_SUCCESS;
}

static NTSTATUS
wipf_callout_install (PDEVICE_OBJECT device)
{
  FWPS_CALLOUT0 c;
  NTSTATUS status;

  if (g_device->active)
    return STATUS_SUCCESS;

  RtlZeroMemory(&c, sizeof(FWPS_CALLOUT0));

  c.notifyFn = wipf_callout_notify;

  /* IPv4 connect filter */

  c.calloutKey = WIPF_GUID_CALLOUT_CONNECT_V4;
  c.classifyFn = wipf_callout_connect_v4;

  status = FwpsCalloutRegister0(device, &c, &g_device->connect4_id);
  if (!NT_SUCCESS(status)) {
    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "WIPF: Register Connect V4: Error: %d\n", status);
    return status;
  }

  /* IPv4 accept filter */

  c.calloutKey = WIPF_GUID_CALLOUT_ACCEPT_V4;
  c.classifyFn = wipf_callout_accept_v4;

  status = FwpsCalloutRegister0(device, &c, &g_device->accept4_id);
  if (!NT_SUCCESS(status)) {
    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "WIPF: Register Accept V4: Error: %d\n", status);
    return status;
  }

  g_device->active = TRUE;

  return STATUS_SUCCESS;
}

static void
wipf_callout_remove (void)
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
wipf_device_create (PDEVICE_OBJECT device, PIRP irp)
{
  NTSTATUS status;

  status = wipf_callout_install(device);

  wipf_request_complete(irp, status);

  return status;
}

static NTSTATUS
wipf_device_close (PDEVICE_OBJECT device, PIRP irp)
{
  UNUSED(device);

  wipf_request_complete(irp, STATUS_SUCCESS);

  return STATUS_SUCCESS;
}

static NTSTATUS
wipf_device_cleanup (PDEVICE_OBJECT device, PIRP irp)
{
  UNUSED(device);

  wipf_callout_remove();

  wipf_conf_ref_set(NULL);

  wipf_request_complete(irp, STATUS_SUCCESS);

  return STATUS_SUCCESS;
}

static void
wipf_device_cancel_pending (PDEVICE_OBJECT device, PIRP irp)
{
  UNUSED(device);

  wipf_buffer_cancel_pending(&g_device->buffer, irp);

  IoReleaseCancelSpinLock(irp->CancelIrql);  /* before IoCompleteRequest()! */

  wipf_request_complete(irp, STATUS_CANCELLED);
}

static NTSTATUS
wipf_device_control (PDEVICE_OBJECT device, PIRP irp)
{
  PIO_STACK_LOCATION irp_stack;
  ULONG_PTR info = 0;
  NTSTATUS status = STATUS_INVALID_PARAMETER;

  UNUSED(device);

  irp_stack = IoGetCurrentIrpStackLocation(irp);

  switch (irp_stack->Parameters.DeviceIoControl.IoControlCode) {
  case WIPF_IOCTL_SETCONF: {
    const PWIPF_CONF conf = irp->AssociatedIrp.SystemBuffer;
    const ULONG len = irp_stack->Parameters.DeviceIoControl.InputBufferLength;

    if (len > WIPF_CONF_SIZE_MIN) {
      PWIPF_CONF_REF conf_ref = wipf_conf_ref_new(conf, len);

      if (conf_ref == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
      } else {
        wipf_conf_ref_set(conf_ref);
        status = STATUS_SUCCESS;
      }
    }
    break;
  }
  case WIPF_IOCTL_GETLOG: {
    PVOID out = irp->AssociatedIrp.SystemBuffer;
    const ULONG out_len = irp_stack->Parameters.DeviceIoControl.OutputBufferLength;

    status = wipf_buffer_xmove(&g_device->buffer, irp, out, out_len, &info);

    if (status == STATUS_PENDING) {
      KIRQL cirq;

      IoMarkIrpPending(irp);

      IoAcquireCancelSpinLock(&cirq);
      IoSetCancelRoutine(irp, wipf_device_cancel_pending);
      IoReleaseCancelSpinLock(cirq);

      return STATUS_PENDING;
    }
    break;
  }
  default: break;
  }

  if (!NT_SUCCESS(status)) {
    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "WIPF: Device Control: Error: %d\n", status);
  }

  wipf_request_complete_info(irp, status, info);

  return status;
}

static void
wipf_driver_unload (PDRIVER_OBJECT driver)
{
  UNICODE_STRING device_link;

  wipf_buffer_close(&g_device->buffer);

  if (g_device->prov_temporary)
    wipf_prov_unregister();

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
  status = IoCreateDevice(driver, sizeof(WIPF_DEVICE), &device_name,
                          WIPF_DEVICE_TYPE, 0, FALSE, &device);

  if (NT_SUCCESS(status)) {
    UNICODE_STRING device_link;

    RtlInitUnicodeString(&device_link, DOS_DEVICE_NAME);
    status = IoCreateSymbolicLink(&device_link, &device_name);

    if (NT_SUCCESS(status)) {
      driver->MajorFunction[IRP_MJ_CREATE] = wipf_device_create;
      driver->MajorFunction[IRP_MJ_CLOSE] = wipf_device_close;
      driver->MajorFunction[IRP_MJ_CLEANUP] = wipf_device_cleanup;
      driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = wipf_device_control;
      driver->DriverUnload = wipf_driver_unload;

      device->Flags |= DO_BUFFERED_IO;

      g_device = device->DeviceExtension;

      RtlZeroMemory(g_device, sizeof(WIPF_DEVICE));

      wipf_buffer_init(&g_device->buffer);

      KeInitializeSpinLock(&g_device->conf_lock);

      // Register filters provider
      {
        BOOL is_temp = FALSE;
        status = wipf_prov_register(FALSE, FALSE, &is_temp);
        g_device->prov_temporary = is_temp;
      }
    }
  }

  if (!NT_SUCCESS(status)) {
    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "WIPF: Entry: Error: %d\n", status);
    wipf_driver_unload(driver);
  }

  return status;
}
