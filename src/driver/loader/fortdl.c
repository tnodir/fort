/* Fort Firewall Driver Loader */

#include "fortdl.h"

#include "../fortutl.h"

#define FORTDL_MAX_FILE_SIZE (4 * 1024 * 1024)

static NTSTATUS fortdl_load_image(PUCHAR data, DWORD dataSize)
{
    NTSTATUS status;

    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: Loader Load Image: %d\n", dataSize);

    return STATUS_SUCCESS;
}

static NTSTATUS fortdl_read_file(HANDLE fileHandle, PUCHAR *outData, DWORD *outSize)
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
    *outSize = dataSize;

    return status;
}

static NTSTATUS fortdl_load_file(PCWSTR driverPath, PUCHAR *outData, DWORD *outSize)
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

    // Read File
    status = fortdl_read_file(fileHandle, outData, outSize);

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
    DWORD dataSize = 0;
    {
        status = fortdl_load_file(context, &data, &dataSize);

        /* Free the allocated driver path */
        ExFreePool(context);
    }

    // Prepare the driver image
    PUCHAR image = NULL;
    if (NT_SUCCESS(status)) {
        status = fortdl_load_image(data, dataSize);

        /* Free the allocated driver file data */
        fortdl_free(data);
    }

    if (!NT_SUCCESS(status)) {
        DbgPrintEx(
                DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: Loader Init: Error: %x\n", status);
    }
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
    status = fort_driver_path(driver, reg_path, &driverPath);

    if (!NT_SUCCESS(status)) {
        DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: Loader Entry: Error: %x\n",
                status);
        return status;
    }

    /* Delay the initialization until other drivers have finished loading */
    IoRegisterDriverReinitialization(driver, fortdl_init, driverPath);

    return STATUS_SUCCESS;
}
