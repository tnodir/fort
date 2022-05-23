/* Fort Firewall Utilities */

#include "fortutl.h"

#define FORT_MAX_FILE_SIZE (4 * 1024 * 1024)

#define FORT_KEY_INFO_PATH_SIZE                                                                    \
    (2 + (MAX_PATH * sizeof(WCHAR)) / sizeof(KEY_VALUE_FULL_INFORMATION))

static WCHAR g_system32PathBuffer[64];
static UNICODE_STRING g_system32Path;

static WCHAR g_systemDrivePathBuffer[32];
static UNICODE_STRING g_systemDrivePath;

static NTSTATUS fort_string_new(ULONG len, PCWSTR src, PUNICODE_STRING outData)
{
    PWSTR buf = fort_mem_alloc_notag(len);
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

static void fort_system_drive_init(PCUNICODE_STRING path)
{
    UNICODE_STRING linkPath = {
        .Length = 2 * sizeof(WCHAR), .MaximumLength = 2 * sizeof(WCHAR), .Buffer = path->Buffer
    };

    WCHAR drivePathBuffer[sizeof(g_systemDrivePathBuffer) / sizeof(WCHAR)];
    UNICODE_STRING drivePath = {
        .Length = 0, .MaximumLength = sizeof(drivePathBuffer), .Buffer = drivePathBuffer
    };

    fort_resolve_link(&linkPath, &drivePath);

    g_systemDrivePath.Length = drivePath.Length;
    g_systemDrivePath.MaximumLength = sizeof(g_systemDrivePathBuffer);
    g_systemDrivePath.Buffer = g_systemDrivePathBuffer;

    RtlDowncaseUnicodeString(&g_systemDrivePath, &drivePath, FALSE);
    g_systemDrivePathBuffer[drivePath.Length / sizeof(WCHAR)] = L'\0';
}

static NTSTATUS fort_system32_path_set(PCUNICODE_STRING path)
{
    if (path->Length > sizeof(g_system32PathBuffer) - sizeof(WCHAR))
        return STATUS_BUFFER_TOO_SMALL;

    g_system32Path.Length = path->Length;
    g_system32Path.MaximumLength = sizeof(g_system32PathBuffer);
    g_system32Path.Buffer = g_system32PathBuffer;

    RtlDowncaseUnicodeString(&g_system32Path, path, FALSE);
    g_system32PathBuffer[path->Length / sizeof(WCHAR)] = L'\0';

    fort_system_drive_init(path);

    return STATUS_SUCCESS;
}

FORT_API void fort_path_prefix_adjust(PUNICODE_STRING path)
{
    if (path->Length < 7)
        return;

    PCWCHAR p = path->Buffer;

    if (p[0] == '\\' && p[1] == '?' && p[2] == '?' && p[3] == '\\' && p[5] == ':') {
        path->Buffer += 4;
        path->Length -= 4 * sizeof(WCHAR);
        path->MaximumLength -= 4 * sizeof(WCHAR);
    }
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
        UNICODE_STRING sys32Path = driverPath;
        sys32Path.Length = (USHORT) ((PCHAR) (sp + 1) /* include the separator */
                - (PCHAR) driverPath.Buffer);

        fort_path_prefix_adjust(&sys32Path);

        status = fort_system32_path_set(&sys32Path);
    } else {
        status = STATUS_OBJECT_PATH_INVALID;
    }

    /* Free the allocated driver path */
    fort_mem_free_notag(driverPath.Buffer);

    return status;
}

FORT_API PUNICODE_STRING fort_system32_path()
{
    return &g_system32Path;
}

FORT_API PUNICODE_STRING fort_system_drive_path()
{
    return &g_systemDrivePath;
}

FORT_API NTSTATUS fort_resolve_link(PUNICODE_STRING linkPath, PUNICODE_STRING outPath)
{
    NTSTATUS status;

    UNICODE_STRING dosRoot;
    RtlInitUnicodeString(&dosRoot, L"\\??");

    OBJECT_ATTRIBUTES objAttr;
    InitializeObjectAttributes(&objAttr, &dosRoot, OBJ_CASE_INSENSITIVE, NULL, NULL);

    HANDLE dirHandle;
    status = ZwOpenDirectoryObject(&dirHandle, DIRECTORY_QUERY, &objAttr);
    if (!NT_SUCCESS(status))
        return status;

    InitializeObjectAttributes(&objAttr, linkPath, OBJ_CASE_INSENSITIVE, dirHandle, NULL);

    HANDLE linkHandle;
    status = ZwOpenSymbolicLinkObject(&linkHandle, SYMBOLIC_LINK_QUERY, &objAttr);
    if (NT_SUCCESS(status)) {
        ULONG outLength = 0;
        status = ZwQuerySymbolicLinkObject(linkHandle, outPath, &outLength);

        if (outLength != 0 && (outLength < 7 || outLength >= outPath->MaximumLength)) {
            status = STATUS_BUFFER_TOO_SMALL;
        }

        if (NT_SUCCESS(status)) {
            if (outPath->Buffer[outLength / sizeof(WCHAR) - 1] == L'\0') {
                outLength -= sizeof(WCHAR); /* chop terminating zero */
            }

            if (outPath->Buffer[outLength / sizeof(WCHAR) - 1] == L'\\') {
                outLength -= sizeof(WCHAR); /* chop terminating backslash */
            }

            outPath->Length = (USHORT) outLength;
        }

        ZwClose(linkHandle);
    }

    ZwClose(dirHandle);

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

BOOL fort_addr_is_local_broadcast(const UINT32 *ip, BOOL isIPv6)
{
    return isIPv6 ? *((const PUINT16) ip) == 0x02FF : *ip == 0xFFFFFFFF;
}
