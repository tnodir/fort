/* Fort Firewall Utilities */

#include "fortutl.h"

#define FORT_MAX_FILE_SIZE (4 * 1024 * 1024)

#define FORT_KEY_INFO_PATH_SIZE                                                                    \
    (2 + (MAX_PATH * sizeof(WCHAR)) / sizeof(KEY_VALUE_FULL_INFORMATION))

static WCHAR g_system32PathBuffer[64];
static UNICODE_STRING g_system32Path;

static NTSTATUS fort_string_new(ULONG len, PCWSTR src, PUNICODE_STRING outData)
{
    PWSTR buf = ExAllocatePool(NonPagedPool, len);
    if (buf == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlCopyMemory(buf, src, len);
    buf[len / sizeof(WCHAR) - sizeof(WCHAR)] = L'\0';

    RtlInitUnicodeString(outData, buf);

    return STATUS_SUCCESS;
}

static NTSTATUS fort_reg_value(HANDLE regKey, PUNICODE_STRING valueName, PUNICODE_STRING outData)
{
    NTSTATUS status;

    KEY_VALUE_FULL_INFORMATION keyInfo[FORT_KEY_INFO_PATH_SIZE];
    ULONG keyInfoSize;

    status = ZwQueryValueKey(
            regKey, valueName, KeyValueFullInformation, keyInfo, sizeof(keyInfo), &keyInfoSize);

    if (NT_SUCCESS(status)) {
        const PUCHAR src = ((const PUCHAR) keyInfo + keyInfo->DataOffset);
        const ULONG len = keyInfo->DataLength + sizeof(WCHAR); // with terminating '\0'

        status = fort_string_new(len, (PCWSTR) src, outData);
    }

    return status;
}

FORT_API NTSTATUS fort_driver_path(
        PDRIVER_OBJECT driver, PUNICODE_STRING regPath, PUNICODE_STRING outPath)
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

    status = fort_reg_value(regKey, &valueName, outPath);

    ZwClose(regKey);
#else
    UNUSED(regPath);

    status = IoQueryFullDriverPath(driver, outPath);
#endif

    return status;
}

static NTSTATUS fort_system32_path_set(PCUNICODE_STRING path, USHORT charCount)
{
    if (charCount >= sizeof(g_system32PathBuffer) / sizeof(WCHAR))
        return STATUS_BUFFER_OVERFLOW;

    const USHORT len = charCount * sizeof(WCHAR);

    RtlCopyMemory(g_system32PathBuffer, path->Buffer, len);
    g_system32PathBuffer[charCount] = L'\0';

    g_system32Path.Length = len;
    g_system32Path.MaximumLength = sizeof(g_system32PathBuffer);
    g_system32Path.Buffer = g_system32PathBuffer;

    return STATUS_SUCCESS;
}

FORT_API NTSTATUS fort_system32_path_init(PDRIVER_OBJECT driver, PUNICODE_STRING regPath)
{
    NTSTATUS status;

    UNICODE_STRING driverPath;
    status = fort_driver_path(driver, regPath, &driverPath);
    if (!NT_SUCCESS(status))
        return status;

    /* Overwrite last symbol to be sure about terminating zero */
    driverPath.Buffer[driverPath.Length / sizeof(WCHAR) - sizeof(WCHAR)] = L'\0';

    /* Find Drivers\ separator */
    WCHAR *sp = wcsrchr(driverPath.Buffer, L'\\');
    if (sp != NULL) {
        *sp = L'\0';

        /* Find System32\ separator */
        sp = wcsrchr(driverPath.Buffer, L'\\');
    }

    if (sp != NULL) {
        const USHORT charCount = (USHORT) (sp + 1 /* include the separator */
                - driverPath.Buffer);

        status = fort_system32_path_set(&driverPath, charCount);
    } else {
        status = STATUS_OBJECT_PATH_INVALID;
    }

    /* Free the allocated driver path */
    ExFreePool(driverPath.Buffer);

    return status;
}

FORT_API PUNICODE_STRING fort_system32_path()
{
    return &g_system32Path;
}

FORT_API NTSTATUS fort_resolve_link(PCWSTR linkPath, PUNICODE_STRING outPath)
{
    NTSTATUS status;

    UNICODE_STRING objName;
    RtlInitUnicodeString(&objName, linkPath);

    OBJECT_ATTRIBUTES objAttr;
    InitializeObjectAttributes(&objAttr, &objName, OBJ_CASE_INSENSITIVE, NULL, NULL);

    HANDLE linkHandle;
    status = ZwOpenSymbolicLinkObject(&linkHandle, GENERIC_READ, &objAttr);
    if (!NT_SUCCESS(status))
        return status;

    ULONG len = outPath->MaximumLength;
    return ZwQuerySymbolicLinkObject(linkHandle, outPath, &len);
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

FORT_API NTSTATUS fort_file_open(PUNICODE_STRING filePath, HANDLE *outHandle)
{
    OBJECT_ATTRIBUTES fileAttr;
    InitializeObjectAttributes(
            &fileAttr, filePath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

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
