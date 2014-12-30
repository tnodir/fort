/* Windows IP Filter Driver */

#define NDIS_WDM	1
#define NDIS630		1

#include <wdm.h>
#include <fwpmk.h>
#include <fwpsk.h>
#include <stddef.h>

#include "../common.h"
#include "wipfdrv.h"

typedef struct wipf_conf_ref {
  UINT32 volatile refcount;

  WIPF_CONF conf;
} WIPF_CONF_REF, *PWIPF_CONF_REF;

typedef struct wipf_driver {
  UINT32 connect4_id;
  UINT32 accept4_id;

  PWIPF_CONF_REF volatile conf_ref;

  KSPIN_LOCK lock;
} WIPF_DRIVER, *PWIPF_DRIVER;

static PWIPF_DRIVER g_driver;

#define WIPF_POOL_TAG	'WIPF'

#define wipf_request_complete(irp, status) \
  do { \
    (irp)->IoStatus.Status = (status); \
    (irp)->IoStatus.Information = 0; \
    IoCompleteRequest((irp), IO_NO_INCREMENT); \
  } while(0)


static PWIPF_CONF_REF
wipf_conf_ref_new (const PWIPF_CONF conf, ULONG len)
{
  const ULONG ref_len = len + offsetof(WIPF_CONF_REF, conf);
  PWIPF_CONF_REF conf_ref = ExAllocatePoolWithTag(NonPagedPool, ref_len, WIPF_POOL_TAG);

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

  KeAcquireSpinLock(&g_driver->lock, &irq);
  {
    const UINT32 refcount = --conf_ref->refcount;

    if (refcount == 0 && conf_ref != g_driver->conf_ref) {
      ExFreePoolWithTag(conf_ref, WIPF_POOL_TAG);
    }
  }
  KeReleaseSpinLock(&g_driver->lock, irq);
}

static PWIPF_CONF_REF
wipf_conf_ref_take (void)
{
  PWIPF_CONF_REF conf_ref;
  KIRQL irq;

  KeAcquireSpinLock(&g_driver->lock, &irq);
  {
    conf_ref = g_driver->conf_ref;
    if (conf_ref) {
      ++conf_ref->refcount;
    }
  }
  KeReleaseSpinLock(&g_driver->lock, irq);

  return conf_ref;
}

static void
wipf_conf_ref_set (PWIPF_CONF_REF conf_ref)
{
  PWIPF_CONF_REF old_conf_ref;
  KIRQL irq;

  old_conf_ref = wipf_conf_ref_take();

  KeAcquireSpinLock(&g_driver->lock, &irq);
  {
    g_driver->conf_ref = conf_ref;
  }
  KeReleaseSpinLock(&g_driver->lock, irq);

  wipf_conf_ref_put(old_conf_ref);
}

static BOOL
wipf_conf_ipblocked (const PWIPF_CONF conf, UINT32 local_ip, UINT32 remote_ip)
{
  UNUSED(conf);
  UNUSED(local_ip);
  UNUSED(remote_ip);

  return TRUE;
}

static NTSTATUS
wipf_callout_classify_v4 (const FWPS_INCOMING_VALUES0 *inFixedValues,
                          const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues,
                          VOID *packet, const FWPS_FILTER0 *filter, UINT64 flowContext,
                          FWPS_CLASSIFY_OUT0 *classifyOut,
                          int localIpField, int remoteIpField)
{
  PWIPF_CONF_REF conf_ref;
  BOOL blocked;

  UNUSED(inMetaValues);
  UNUSED(packet);
  UNUSED(filter);
  UNUSED(flowContext);

  conf_ref = wipf_conf_ref_take();
  if (conf_ref == NULL)
    return STATUS_SUCCESS;

  /* Check IP */
  {
    const UINT32 local_ip = inFixedValues->incomingValue[localIpField].value.uint32;
    const UINT32 remote_ip = inFixedValues->incomingValue[remoteIpField].value.uint32;

    blocked = wipf_conf_ipblocked(&conf_ref->conf, local_ip, remote_ip);
  }

  wipf_conf_ref_put(conf_ref);

  if (blocked) {
    if (filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT) {
      classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
    }
    classifyOut->actionType = FWP_ACTION_BLOCK;

    return STATUS_SUCCESS;
  }

  classifyOut->actionType = FWP_ACTION_CONTINUE;

  return STATUS_SUCCESS;
}

static NTSTATUS
wipf_callout_connect_v4 (const FWPS_INCOMING_VALUES0 *inFixedValues,
                         const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues,
                         VOID *packet, const FWPS_FILTER0 *filter, UINT64 flowContext,
                         FWPS_CLASSIFY_OUT0 *classifyOut)
{
  return wipf_callout_classify_v4(inFixedValues, inMetaValues, packet, filter, flowContext, classifyOut,
      FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_LOCAL_ADDRESS,
      FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_REMOTE_ADDRESS);
}

static NTSTATUS
wipf_callout_accept_v4 (const FWPS_INCOMING_VALUES0 *inFixedValues,
                        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues,
                        VOID *packet, const FWPS_FILTER0 *filter, UINT64 flowContext,
                        FWPS_CLASSIFY_OUT0 *classifyOut)
{
  return wipf_callout_classify_v4(inFixedValues, inMetaValues, packet, filter, flowContext, classifyOut,
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
  FWPS_CALLOUT0 c = {0};
  NTSTATUS status;

  DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_TRACE_LEVEL, "wipf: > wipf_callout_install()\n");

  c.notifyFn = wipf_callout_notify;

  /* IPv4 connect filter */

  c.calloutKey = WIPF_CONNECT_CALLOUT_V4;
  c.classifyFn = wipf_callout_connect_v4;

  status = FwpsCalloutRegister0(device, &c, &g_driver->connect4_id);
  if (!NT_SUCCESS(status)) {
    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_TRACE_LEVEL, "wipf: Register Connect V4: Error: %d\n", status);
    return status;
  }

  /* IPv4 accept filter */

  c.calloutKey = WIPF_ACCEPT_CALLOUT_V4;
  c.classifyFn = wipf_callout_accept_v4;

  status = FwpsCalloutRegister0(device, &c, &g_driver->accept4_id);
  if (!NT_SUCCESS(status)) {
    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_TRACE_LEVEL, "wipf: Register Accept V4: Error: %d\n", status);
    return status;
  }

  DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_TRACE_LEVEL, "wipf: < wipf_callout_install()\n");
  return STATUS_SUCCESS;
}

static NTSTATUS
wipf_driver_complete (PDEVICE_OBJECT device, PIRP irp)
{
  UNUSED(device);

  DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_TRACE_LEVEL, "wipf: > wipf_driver_complete()\n");

  wipf_request_complete(irp, STATUS_SUCCESS);

  DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_TRACE_LEVEL, "wipf: < wipf_driver_complete()\n");
  return STATUS_SUCCESS;
}

static NTSTATUS
wipf_driver_cleanup (PDEVICE_OBJECT device, PIRP irp)
{
  UNUSED(device);

  DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_TRACE_LEVEL, "wipf: > wipf_driver_cleanup()\n");

  wipf_conf_ref_set(NULL);

  wipf_request_complete(irp, STATUS_SUCCESS);

  DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_TRACE_LEVEL, "wipf: < wipf_driver_cleanup()\n");
  return STATUS_SUCCESS;
}

static NTSTATUS
wipf_driver_control (PDEVICE_OBJECT device, PIRP irp)
{
  PIO_STACK_LOCATION irp_stack;
  NTSTATUS status = STATUS_INVALID_PARAMETER;

  UNUSED(device);

  DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_TRACE_LEVEL, "wipf: > wipf_driver_control()\n");

  irp_stack = IoGetCurrentIrpStackLocation(irp);

  switch (irp_stack->Parameters.DeviceIoControl.IoControlCode) {
  case WIPF_IOCTL_SETCONF: {
    const PWIPF_CONF conf = irp->AssociatedIrp.SystemBuffer;
    const ULONG len = irp_stack->Parameters.DeviceIoControl.InputBufferLength;

    if (len >= sizeof(WIPF_CONF)) {
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
  default: break;
  }

  if (!NT_SUCCESS(status)) {
    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "wipf: Control: Error: %d\n", status);
  }

  wipf_request_complete(irp, status);

  DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_TRACE_LEVEL, "wipf: < wipf_driver_control()\n");
  return status;
}

static void
wipf_driver_unload (PDRIVER_OBJECT driver)
{
  UNICODE_STRING device_link;
  NTSTATUS status;

  DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_TRACE_LEVEL, "wipf: > wipf_driver_unload()\n");

  if (g_driver->connect4_id) {
    status = FwpsCalloutUnregisterById0(g_driver->connect4_id);
    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_TRACE_LEVEL, "wipf: Unregister Connect V4: %d\n", status);
  }

  if (g_driver->accept4_id) {
    status = FwpsCalloutUnregisterById0(g_driver->accept4_id);
    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_TRACE_LEVEL, "wipf: Unregister Accept V4: %d\n", status);
  }

  RtlInitUnicodeString(&device_link, DOS_DEVICE_NAME);
  IoDeleteSymbolicLink(&device_link);

  IoDeleteDevice(driver->DeviceObject);

  DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_TRACE_LEVEL, "wipf: < wipf_driver_unload()\n");
}

NTSTATUS
DriverEntry (PDRIVER_OBJECT driver, PUNICODE_STRING reg_path)
{
  UNICODE_STRING device_name;
  PDEVICE_OBJECT device;
  NTSTATUS status;

  UNUSED(reg_path);

  DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_TRACE_LEVEL, "wipf: > DriverEntry()\n");

  RtlInitUnicodeString(&device_name, NT_DEVICE_NAME);
  status = IoCreateDevice(driver, sizeof(WIPF_DRIVER), &device_name,
                          WIPF_DEVICE_TYPE, 0, FALSE, &device);

  if (NT_SUCCESS(status)) {
    UNICODE_STRING device_link;

    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_TRACE_LEVEL, "wipf: Device created\n");

    RtlInitUnicodeString(&device_link, DOS_DEVICE_NAME);
    status = IoCreateSymbolicLink(&device_link, &device_name);

    if (NT_SUCCESS(status)) {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_TRACE_LEVEL, "wipf: Device link created\n");

      driver->MajorFunction[IRP_MJ_CREATE] =
        driver->MajorFunction[IRP_MJ_CLOSE] = wipf_driver_complete;
      driver->MajorFunction[IRP_MJ_CLEANUP] = wipf_driver_cleanup;
      driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = wipf_driver_control;
      driver->DriverUnload = wipf_driver_unload;

      device->Flags |= DO_BUFFERED_IO;

      g_driver = device->DeviceExtension;

      RtlZeroMemory(g_driver, sizeof(WIPF_DRIVER));

      KeInitializeSpinLock(&g_driver->lock);

      status = wipf_callout_install(device);
    }
  }

  if (!NT_SUCCESS(status)) {
    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "wipf: Entry: Error: %d\n", status);
    wipf_driver_unload(driver);
  }

  DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_TRACE_LEVEL, "wipf: < DriverEntry()\n");
  return status;
}
