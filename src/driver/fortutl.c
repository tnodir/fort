/* Fort Firewall Utilities */

#include "fortutl.h"

#define FORT_MAX_FILE_SIZE (4 * 1024 * 1024)

#define FORT_KEY_INFO_PATH_SIZE                                                                    \
    (2 + (MAX_PATH * sizeof(WCHAR)) / sizeof(KEY_VALUE_FULL_INFORMATION))

static NTSTATUS fort_reg_value(HANDLE regKey, PUNICODE_STRING valueName, PWSTR *outData)
{
    NTSTATUS status;

    KEY_VALUE_FULL_INFORMATION keyInfo[FORT_KEY_INFO_PATH_SIZE];
    ULONG keyInfoSize;

    status = ZwQueryValueKey(
            regKey, valueName, KeyValueFullInformation, keyInfo, sizeof(keyInfo), &keyInfoSize);

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

FORT_API NTSTATUS fort_file_read(HANDLE fileHandle, ULONG poolTag, PUCHAR *outData, DWORD *outSize)
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
                || fileInfo.EndOfFile.LowPart > FORT_MAX_FILE_SIZE)
            return STATUS_FILE_NOT_SUPPORTED;

        fileSize = fileInfo.EndOfFile.LowPart;
    }

    // Allocate Buffer
    PUCHAR data = fort_mem_alloc(fileSize, poolTag);
    if (data == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    // Read File
    DWORD dataSize = 0;
    do {
        IO_STATUS_BLOCK statusBlock;
        status = ZwReadFile(fileHandle, NULL, NULL, NULL, &statusBlock, data + dataSize,
                fileSize - dataSize, NULL, NULL);

        if (!NT_SUCCESS(status) || statusBlock.Information == 0) {
            fort_mem_free(data, poolTag);
            return NT_SUCCESS(status) ? STATUS_FILE_NOT_AVAILABLE : status;
        }

        dataSize += (DWORD) statusBlock.Information;
    } while (dataSize < fileSize);

    *outData = data;
    *outSize = dataSize;

    return status;
}

FORT_API NTSTATUS fort_file_open(PCWSTR filePath, HANDLE *outHandle)
{
    UNICODE_STRING path;
    RtlInitUnicodeString(&path, filePath);

    OBJECT_ATTRIBUTES fileAttr;
    InitializeObjectAttributes(
            &fileAttr, &path, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    IO_STATUS_BLOCK statusBlock;
    return ZwOpenFile(outHandle, GENERIC_READ | SYNCHRONIZE, &fileAttr, &statusBlock, 0,
            FILE_SYNCHRONOUS_IO_NONALERT);
}

USHORT fort_le_u16_read(const char *cp, int offset)
{
    USHORT v = *((USHORT *) (cp + offset));
#ifdef FORT_BIG_ENDIAN
    RtlUshortByteSwap(v);
#endif
    return v;
}

DWORD fort_le_u32_read(const char *cp, int offset)
{
    DWORD v = *((DWORD *) (cp + offset));
#ifdef FORT_BIG_ENDIAN
    RtlUlongByteSwap(v);
#endif
    return v;
}
