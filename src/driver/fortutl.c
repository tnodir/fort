/* Fort Firewall Utilities */

#include "fortutl.h"

#include "../version/fort_version_l.h"

#define FORT_UTL_POOL_TAG 'UwfF'

#define FORT_MAX_FILE_SIZE (4 * 1024 * 1024)

#define FORT_KEY_INFO_PATH_SIZE                                                                    \
    (2 * sizeof(KEY_VALUE_PARTIAL_INFORMATION) + (MAX_PATH * sizeof(WCHAR)))

#define FORT_KERNEL_STACK_SIZE (8 * 1024)

typedef struct fort_expand_stack_arg
{
    FORT_EXPAND_STACK_FUNC func;
    PVOID param;
    NTSTATUS status;
} FORT_EXPAND_STACK_ARG, *PFORT_EXPAND_STACK_ARG;

static WCHAR g_system32PathBuffer[64];
static UNICODE_STRING g_system32Path;

static WCHAR g_systemDrivePathBuffer[32];
static UNICODE_STRING g_systemDrivePath;

static NTSTATUS fort_string_new(ULONG len, PCWSTR src, PUNICODE_STRING outData)
{
    PWSTR buf = fort_mem_alloc(len, FORT_UTL_POOL_TAG);
    if (buf == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlCopyMemory(buf, src, len);
    buf[len / sizeof(WCHAR) - sizeof(WCHAR)] = L'\0';

    RtlInitUnicodeString(outData, buf);

    return STATUS_SUCCESS;
}

static NTSTATUS fort_reg_value_path(
        HANDLE regKey, PUNICODE_STRING valueName, PUNICODE_STRING outData)
{
    NTSTATUS status;

    PKEY_VALUE_PARTIAL_INFORMATION keyInfo =
            fort_mem_alloc(FORT_KEY_INFO_PATH_SIZE, FORT_UTL_POOL_TAG);
    if (keyInfo == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    ULONG keyInfoSize = 0;

    status = ZwQueryValueKey(regKey, valueName, KeyValuePartialInformation, keyInfo,
            FORT_KEY_INFO_PATH_SIZE, &keyInfoSize);

    if (NT_SUCCESS(status)) {
        const PUCHAR src = keyInfo->Data;
        const ULONG len = keyInfo->DataLength + sizeof(WCHAR); /* with terminating '\0' */

        status = fort_string_new(len, (PCWSTR) src, outData);
    }

    fort_mem_free(keyInfo, FORT_UTL_POOL_TAG);

    return status;
}

static NTSTATUS fort_reg_value_dword(HANDLE regKey, PUNICODE_STRING valueName, PDWORD outData)
{
    NTSTATUS status;

    UCHAR buf[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD)];
    PKEY_VALUE_PARTIAL_INFORMATION keyInfo = (PVOID) buf;

    ULONG keyInfoSize = 0;

    status = ZwQueryValueKey(
            regKey, valueName, KeyValuePartialInformation, keyInfo, sizeof(buf), &keyInfoSize);

    if (NT_SUCCESS(status)) {
        const PUCHAR src = keyInfo->Data;

        *outData = *((PDWORD) src);
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

    status = ZwOpenKey(&regKey, KEY_READ, &objectAttr);
    if (!NT_SUCCESS(status))
        return status;

    UNICODE_STRING valueName;
    RtlInitUnicodeString(&valueName, L"ImagePath");

    status = fort_reg_value_path(regKey, &valueName, outPath);

    ZwClose(regKey);
#else
    UNUSED(regPath);

    status = IoQueryFullDriverPath(driver, outPath);
#endif

    return status;
}

FORT_API DWORD fort_reg_flag(PCWSTR name)
{
    NTSTATUS status;

    UNICODE_STRING regPath;
    RtlInitUnicodeString(&regPath, L"\\Registry\\Machine\\Software\\" APP_NAME_L);

    HANDLE regKey;
    OBJECT_ATTRIBUTES objectAttr;

    InitializeObjectAttributes(
            &objectAttr, &regPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    status = ZwOpenKey(&regKey, KEY_READ, &objectAttr);
    if (!NT_SUCCESS(status))
        return status;

    UNICODE_STRING valueName;
    RtlInitUnicodeString(&valueName, name);

    DWORD flagValue = 0;

    status = fort_reg_value_dword(regKey, &valueName, &flagValue);

    ZwClose(regKey);

    return flagValue;
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

#define FORT_PATH_PREFIX_DEVICE_ALIAS_LEN 4

inline static BOOL fort_path_prefix_is_device_alias(PUNICODE_STRING path)
{
    PCWCHAR p = path->Buffer;

    return (p[0] == '\\' && p[1] == '?' && p[2] == '?' && p[3] == '\\' && p[5] == ':');
}

FORT_API void fort_path_prefix_adjust(PUNICODE_STRING path)
{
    if (path->Length < 7)
        return;

    if (fort_path_prefix_is_device_alias(path)) {
        path->Buffer += FORT_PATH_PREFIX_DEVICE_ALIAS_LEN;
        path->Length -= FORT_PATH_PREFIX_DEVICE_ALIAS_LEN * sizeof(WCHAR);
        path->MaximumLength -= FORT_PATH_PREFIX_DEVICE_ALIAS_LEN * sizeof(WCHAR);
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

FORT_API PUNICODE_STRING fort_system32_path(void)
{
    return &g_system32Path;
}

FORT_API PUNICODE_STRING fort_system_drive_path(void)
{
    return &g_systemDrivePath;
}

inline static NTSTATUS fort_resolve_link_handle(HANDLE linkHandle, PUNICODE_STRING outPath)
{
    NTSTATUS status;

    ULONG outLength = 0;
    status = ZwQuerySymbolicLinkObject(linkHandle, outPath, &outLength);

    if (outLength != 0) {
        if (outLength < 7)
            return STATUS_SYMLINK_CLASS_DISABLED;

        if (outLength >= outPath->MaximumLength)
            return STATUS_BUFFER_TOO_SMALL;
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

    return status;
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
        status = fort_resolve_link_handle(linkHandle, outPath);

        ZwClose(linkHandle);
    }

    ZwClose(dirHandle);

    return status;
}

inline static NTSTATUS fort_file_size(HANDLE fileHandle, DWORD *fileSize)
{
    NTSTATUS status;

    IO_STATUS_BLOCK statusBlock;
    FILE_STANDARD_INFORMATION fileInfo;
    status = ZwQueryInformationFile(
            fileHandle, &statusBlock, &fileInfo, sizeof(fileInfo), FileStandardInformation);

    if (!NT_SUCCESS(status))
        return status;

    if (fileInfo.EndOfFile.HighPart != 0)
        return STATUS_FILE_NOT_SUPPORTED;

    if (fileInfo.EndOfFile.LowPart <= 0)
        return STATUS_FILE_IS_OFFLINE;

    if (fileInfo.EndOfFile.LowPart > FORT_MAX_FILE_SIZE)
        return STATUS_FILE_TOO_LARGE;

    *fileSize = fileInfo.EndOfFile.LowPart;

    return STATUS_SUCCESS;
}

inline static NTSTATUS fort_file_read_data(
        HANDLE fileHandle, DWORD fileSize, PUCHAR data, DWORD *outSize)
{
    NTSTATUS status = STATUS_SUCCESS;

    DWORD dataSize = 0;
    do {
        IO_STATUS_BLOCK statusBlock;
        status = ZwReadFile(fileHandle, NULL, NULL, NULL, &statusBlock, data + dataSize,
                fileSize - dataSize, NULL, NULL);

        if (!NT_SUCCESS(status))
            break;

        if (statusBlock.Information == 0) {
            status = STATUS_FILE_NOT_AVAILABLE;
            break;
        }

        dataSize += (DWORD) statusBlock.Information;
    } while (dataSize < fileSize);

    *outSize = dataSize;

    return status;
}

FORT_API NTSTATUS fort_file_read(HANDLE fileHandle, ULONG poolTag, PUCHAR *outData, DWORD *outSize)
{
    NTSTATUS status;

    /* Get File Size */
    DWORD fileSize = 0;
    status = fort_file_size(fileHandle, &fileSize);
    if (!NT_SUCCESS(status))
        return status;

    /* Allocate Buffer */
    PUCHAR data = fort_mem_alloc(fileSize, poolTag);
    if (data == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Read File */
    status = fort_file_read_data(fileHandle, fileSize, data, outSize);
    if (!NT_SUCCESS(status)) {
        fort_mem_free(data, poolTag);
        return status;
    }

    *outData = data;

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

FORT_API USHORT fort_le_u16_read(const char *cp, int offset)
{
    USHORT v = *((USHORT *) (cp + offset));
#if FORT_BIG_ENDIAN
    RtlUshortByteSwap(v);
#endif
    return v;
}

FORT_API DWORD fort_le_u32_read(const char *cp, int offset)
{
    DWORD v = *((DWORD *) (cp + offset));
#if FORT_BIG_ENDIAN
    RtlUlongByteSwap(v);
#endif
    return v;
}

FORT_API void fort_ascii_downcase(PUNICODE_STRING dst, PCUNICODE_STRING src)
{
    PWCHAR dp = dst->Buffer;
    PCWCH cp = src->Buffer;

    USHORT len =
            ((dst->MaximumLength < src->Length) ? dst->MaximumLength : src->Length) / sizeof(WCHAR);

    while (len-- > 0) {
        const WCHAR c = *cp++;

        const BOOL isUpperAscii = (c >= 'A' && c <= 'Z');
        const WCHAR d = c | (isUpperAscii ? 32 : 0);

        *dp++ = d;
    }
}

FORT_API BOOL fort_addr_is_local_broadcast(const UINT32 *ip, BOOL isIPv6)
{
    if (isIPv6) {
        const ip6_addr_t *ip6 = (const ip6_addr_t *) ip;
        return ip6->addr16[0] == 0x2FF;
    }

    return *ip == 0xFFFFFFFF;
}

inline static UINT32 fort_bits_duplicate8(UINT32 v)
{
    return ((v & 1) | ((v & 1) << 1)) /* 1-st bit */
            | (((v & (1 << 1)) | ((v & (1 << 1)) << 1)) << 1) /* 2-nd bit */
            | (((v & (1 << 2)) | ((v & (1 << 2)) << 1)) << 2) /* 3-rd bit */
            | (((v & (1 << 3)) | ((v & (1 << 3)) << 1)) << 3) /* 4 bit */
            | (((v & (1 << 4)) | ((v & (1 << 4)) << 1)) << 4) /* 5 bit */
            | (((v & (1 << 5)) | ((v & (1 << 5)) << 1)) << 5) /* 6 bit */
            | (((v & (1 << 6)) | ((v & (1 << 6)) << 1)) << 6) /* 7 bit */
            | (((v & (1 << 7)) | ((v & (1 << 7)) << 1)) << 7) /* 8 bit */
            ;
}

FORT_API UINT32 fort_bits_duplicate16(UINT16 num)
{
    return fort_bits_duplicate8(num & 0xFF) | (fort_bits_duplicate8(num >> 8) << 16);
}

FORT_API void fort_irp_set_cancel_routine(PIRP irp, PDRIVER_CANCEL routine)
{
    if (irp == NULL)
        return;

    KIRQL cirq;
    IoAcquireCancelSpinLock(&cirq);
    {
        IoSetCancelRoutine(irp, routine);
    }
    IoReleaseCancelSpinLock(cirq);
}

static void NTAPI fort_expand_stack_call(PVOID context)
{
    PFORT_EXPAND_STACK_ARG arg = context;

    arg->status = arg->func(arg->param);
}

FORT_API NTSTATUS fort_expand_stack(FORT_EXPAND_STACK_FUNC func, PVOID param)
{
    FORT_EXPAND_STACK_ARG arg = { .func = func, .param = param };

    const NTSTATUS status =
            KeExpandKernelStackAndCallout(&fort_expand_stack_call, &arg, FORT_KERNEL_STACK_SIZE);

    return !NT_SUCCESS(status) ? status : arg.status;
}
