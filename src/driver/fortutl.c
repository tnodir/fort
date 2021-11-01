/* Fort Firewall Utilities */

#include "fortutl.h"

#define FORT_UTIL_POOL_TAG 'UwfF'

FORT_API NTSTATUS fort_reg_value(HANDLE regKey, PUNICODE_STRING valueName, PWSTR *outData)
{
    NTSTATUS status;

    ULONG keyInfoSize;
    status = ZwQueryValueKey(regKey, valueName, KeyValueFullInformation, NULL, 0, &keyInfoSize);
    if (status != STATUS_BUFFER_TOO_SMALL || status != STATUS_BUFFER_OVERFLOW)
        return status;

    PKEY_VALUE_FULL_INFORMATION keyInfo = fort_mem_alloc(keyInfoSize, FORT_UTIL_POOL_TAG);
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

            *outData = buf;
        }
    }

    fort_mem_free(keyInfo, FORT_UTIL_POOL_TAG);

    return status;
}

FORT_API NTSTATUS fort_driver_path(PDRIVER_OBJECT driver, PUNICODE_STRING regPath, PWSTR *outPath)
{
    NTSTATUS status;

#if defined(FORT_WIN7_COMPAT)
    UNUSED(driver);

    HANDLE regKey;
    OBJECT_ATTRIBUTES objectAttr;

    InitializeObjectAttributes(
            &objectAttr, regPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    status = ZwOpenKey(&regKey, KEY_QUERY_VALUE, &objectAttr);
    if (!NT_SUCCESS(status))
        return status;

    UNICODE_STRING valueName;
    RtlInitUnicodeString(&valueName, L"ImagePath");

    status = fortdl_reg_value(regKey, &valueName, outPath);

    ZwClose(regKey);
#else
    UNUSED(regPath);

    UNICODE_STRING path;
    status = IoQueryFullDriverPath(driver, &path);

    if (NT_SUCCESS(status)) {
        *outPath = path.Buffer;
    }
#endif

    return status;
}
