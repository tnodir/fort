/* Fort Firewall Driver Loader */

#include "fortdl.h"

#define FORTDL_MAX_FILE_SIZE (4 * 1024 * 1024)

static NTSTATUS fortdl_read_file(HANDLE fileHandle, PUCHAR *outData)
{
    NTSTATUS status;

    // Get File Size
    DWORD fileSize = 0;
    {
        IO_STATUS_BLOCK statusBlock;
        FILE_STANDARD_INFORMATION fileInfo;
        status = ZwQueryInformationFile(
                fileHandle, &statusBlock, &fileInfo, sizeof(fileInfo), FileStandardInformation);

        if (!NT_SUCCESS(status))
            return status;

        if (fileInfo.EndOfFile.HighPart != 0 || fileInfo.EndOfFile.LowPart <= 0
                || fileInfo.EndOfFile.LowPart > FORTDL_MAX_FILE_SIZE)
            return STATUS_FILE_NOT_SUPPORTED;

        fileSize = fileInfo.EndOfFile.LowPart;
    }

    // Allocate Buffer
    PUCHAR data = fortdl_alloc(fileSize);
    if (data == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    // Read File
    DWORD dataSize = 0;
    do {
        IO_STATUS_BLOCK statusBlock;
        status = ZwReadFile(fileHandle, NULL, NULL, NULL, &statusBlock, data + dataSize,
                fileSize - dataSize, NULL, NULL);

        if (!NT_SUCCESS(status) || statusBlock.Information == 0) {
            fortdl_free(data);
            return NT_SUCCESS(status) ? STATUS_FILE_NOT_AVAILABLE : status;
        }

        dataSize += (DWORD) statusBlock.Information;
    } while (dataSize < fileSize);

    *outData = data;

    return status;
}

static NTSTATUS fortdl_load_file(PCWSTR driverPath, PUCHAR *outData)
{
    NTSTATUS status;

    DbgPrintEx(
            DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: Loader Load File: %w\n", driverPath);

    // Open File
    HANDLE fileHandle;
    {
        UNICODE_STRING path;
        RtlInitUnicodeString(&path, driverPath);

        OBJECT_ATTRIBUTES fileAttr;
        InitializeObjectAttributes(
                &fileAttr, &path, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

        IO_STATUS_BLOCK statusBlock;
        status = ZwOpenFile(&fileHandle, GENERIC_READ | SYNCHRONIZE, &fileAttr, &statusBlock, 0,
                FILE_SYNCHRONOUS_IO_NONALERT);
        if (!NT_SUCCESS(status))
            return status;
    }

    status = fortdl_read_file(fileHandle, outData);

    ZwClose(fileHandle);

    return status;
}

static void fortdl_init(PDRIVER_OBJECT driver, PVOID context, ULONG count)
{
    NTSTATUS status;

    UNUSED(context);
    UNUSED(count);

    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: Loader Init: %d\n", count);

    /* Load the driver file */
    PUCHAR data = NULL;
    {
        status = fortdl_load_file(context, &data);

        /* Free the allocated driver path */
        ExFreePool(context);
    }

    if (!NT_SUCCESS(status)) {
        DbgPrintEx(
                DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: Loader Init: Error: %x\n", status);
    }
}

#if defined(FORT_WIN7_COMPAT)

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

#endif

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
