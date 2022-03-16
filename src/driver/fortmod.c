/* Fort Firewall Kernel Modules */

#include "fortmod.h"

#define FORT_MODULE_POOL_TAG 'MwfF'

#if defined(FORT_DRIVER)
#    include <aux_klib.h>
#endif

FORT_API PIMAGE_NT_HEADERS GetModuleNtHeaders(PUCHAR codeBase)
{
    return ((PIMAGE_NT_HEADERS) & ((PUCHAR) (codeBase))[((PIMAGE_DOS_HEADER) codeBase)->e_lfanew]);
}

FORT_API NTSTATUS GetModuleInfo(PLOADEDMODULE pModule, LPCSTR name,
        const PAUX_MODULE_EXTENDED_INFO modules, DWORD modulesCount)
{
    PAUX_MODULE_EXTENDED_INFO module = modules;

    for (DWORD i = 0; i < modulesCount; ++i, ++module) {
        LPCSTR libName = (LPCSTR) &module->FullPathName[module->FileNameOffset];
        if (_stricmp(name, libName) == 0) {
            pModule->codeBase = module->BasicInfo.ImageBase;
            return STATUS_SUCCESS;
        }
    }

    return STATUS_PROCEDURE_NOT_FOUND;
}

FORT_API NTSTATUS GetModuleInfoList(PAUX_MODULE_EXTENDED_INFO *outModules, DWORD *outModulesCount)
{
    NTSTATUS status;

    status = AuxKlibInitialize();
    if (!NT_SUCCESS(status))
        return status;

    ULONG size = 0;
    status = AuxKlibQueryModuleInformation(&size, sizeof(AUX_MODULE_EXTENDED_INFO), NULL);
    if (!NT_SUCCESS(status) || size == 0)
        return NT_SUCCESS(status) ? STATUS_INVALID_BUFFER_SIZE : status;

    PUCHAR data = fort_mem_alloc(size, FORT_MODULE_POOL_TAG);
    if (data == NULL)
        return STATUS_NO_MEMORY;

    status = AuxKlibQueryModuleInformation(&size, sizeof(AUX_MODULE_EXTENDED_INFO), data);
    if (!NT_SUCCESS(status)) {
        fort_mem_free(data, FORT_MODULE_POOL_TAG);
        return status;
    }

    *outModules = (PAUX_MODULE_EXTENDED_INFO) data;
    *outModulesCount = size / sizeof(AUX_MODULE_EXTENDED_INFO);

    return status;
}

void FreeModuleInfoList(PAUX_MODULE_EXTENDED_INFO modules)
{
    fort_mem_free(modules, FORT_MODULE_POOL_TAG);
}

static int ModuleGetProcIndex(
        const PUCHAR codeBase, const PIMAGE_EXPORT_DIRECTORY exports, LPCSTR funcName)
{
    if (HIWORD(funcName) == 0) {
        /* Load function by ordinal value */
        return LOWORD(funcName) - exports->Base;
    }

    /* Search function name in list of exported names */
    DWORD *nameRef = (DWORD *) (codeBase + exports->AddressOfNames);
    WORD *ordinal = (WORD *) (codeBase + exports->AddressOfNameOrdinals);

    for (DWORD i = 0; i < exports->NumberOfNames; ++i, ++nameRef, ++ordinal) {
        if (strcmp(funcName, (const char *) (codeBase + *nameRef)) == 0) {
            return *ordinal;
        }
    }

    return -1;
}

/* Retrieve address of an exported function from the loaded module. */
FORT_API FARPROC ModuleGetProcAddress(PLOADEDMODULE pModule, LPCSTR funcName)
{
    const PUCHAR codeBase = pModule->codeBase;
    const PIMAGE_NT_HEADERS pHeaders = GetModuleNtHeaders(codeBase);

    const PIMAGE_DATA_DIRECTORY directory =
            &(pHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]);
    if (directory->Size == 0)
        return NULL; /* no export table found */

    const PIMAGE_EXPORT_DIRECTORY exports =
            (PIMAGE_EXPORT_DIRECTORY) (codeBase + directory->VirtualAddress);
    if (exports->NumberOfNames == 0 || exports->NumberOfFunctions == 0)
        return NULL; /* Our modules must export 3 functions. */

    const int idx = ModuleGetProcIndex(codeBase, exports, funcName);

    if (idx < 0 || idx > (int) exports->NumberOfFunctions)
        return NULL; /* exported symbol not found or name <-> ordinal number don't match */

    /* AddressOfFunctions contains the RVAs to the "real" functions */
    return (FARPROC) (PVOID) (codeBase
            + *(DWORD *) (codeBase + exports->AddressOfFunctions + (SIZE_T) idx * 4));
}
