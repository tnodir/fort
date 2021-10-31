/* Fort Firewall Driver Loader */

#include "fortdl.h"

static void fortdl_load(PCUNICODE_STRING driverPath)
{
    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: Loader Load: %w\n", driverPath);
}

static void fortdl_init(PDRIVER_OBJECT driver, PVOID context, ULONG count)
{
    PWSTR driverPath = context;

    UNUSED(context);
    UNUSED(count);

    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: Loader Init: %d\n", count);

    /* Load the driver */
    {
        UNICODE_STRING path;

        RtlInitUnicodeString(&path, driverPath);
        fortdl_load(&path);
    }

    /* Free the allocated driver path */
    ExFreePool(context);
}

static NTSTATUS fortdl_reg_value(HANDLE regKey, PUNICODE_STRING valueName, PWSTR *out_path)
{
    NTSTATUS status;

    ULONG keyInfoSize;
    status = ZwQueryValueKey(regKey, valueName, KeyValueFullInformation, NULL, 0, &keyInfoSize);
    if (status != STATUS_BUFFER_TOO_SMALL || status != STATUS_BUFFER_OVERFLOW)
        return status;

    PKEY_VALUE_FULL_INFORMATION keyInfo = fortdl_alloc(keyInfoSize);
    if (keyInfo == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    status = ZwQueryValueKey(
            regKey, valueName, KeyValueFullInformation, keyInfo, keyInfoSize, &keyInfoSize);

    if (NT_SUCCESS(status)) {
        const PUCHAR src = ((const PUCHAR) keyInfo + keyInfo->DataOffset);
        const ULONG len = keyInfo->DataLength;

        PWSTR buf = ExAllocatePool(NonPagedPool, len);
        if (buf == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
        } else {
            RtlCopyMemory(buf, src, len);

            *out_path = buf;
        }
    }

    fortdl_free(keyInfo);

    return status;
}

static NTSTATUS fortdl_driver_path(PDRIVER_OBJECT driver, PUNICODE_STRING reg_path, PWSTR *out_path)
{
    NTSTATUS status;

#if defined(FORT_WIN7_COMPAT)
    UNUSED(driver);

    HANDLE regKey;
    OBJECT_ATTRIBUTES objectAttr;

    InitializeObjectAttributes(
            &objectAttr, reg_path, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    status = ZwOpenKey(&regKey, KEY_QUERY_VALUE, &objectAttr);
    if (!NT_SUCCESS(status))
        return status;

    UNICODE_STRING valueName;
    RtlInitUnicodeString(&valueName, L"ImagePath");

    status = fortdl_reg_value(regKey, &valueName, out_path);

    ZwClose(regKey);
#else
    UNUSED(reg_path);

    UNICODE_STRING path;
    status = IoQueryFullDriverPath(driver, &path);

    if (NT_SUCCESS(status)) {
        *out_path = path.Buffer;
    }
#endif

    return status;
}

NTSTATUS
#if defined(FORT_DRIVER)
DriverEntry
#else
DriverLoaderEntry
#endif
        (PDRIVER_OBJECT driver, PUNICODE_STRING reg_path)
{
    NTSTATUS status;

    PWSTR driverPath = NULL;
    status = fortdl_driver_path(driver, reg_path, &driverPath);

    if (!NT_SUCCESS(status)) {
        DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: Loader Entry: Error: %x\n",
                status);
        return status;
    }

    /* Delay the initialization until other drivers have finished loading */
    IoRegisterDriverReinitialization(driver, fortdl_init, driverPath);

    return STATUS_SUCCESS;
}
