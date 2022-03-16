#ifndef FORTMM_IMP_H
#define FORTMM_IMP_H

#include "fortdl.h"

#include "../fortmod.h"

#if defined(__cplusplus)
extern "C" {
#endif

struct fort_module_imp;
typedef struct fort_module_imp *PFORT_MODULE_IMP;

typedef ULONG (*DbgPrintExProc)(ULONG componentId, ULONG level, PCSTR format, ...);

typedef int(__cdecl *StrICmpProc)(const char *s1, const char *s2);

typedef NTSTATUS (*GetModuleInfoProc)(PLOADEDMODULE pModule, LPCSTR name,
        const PAUX_MODULE_EXTENDED_INFO modules, DWORD modulesCount);

typedef NTSTATUS (*GetModuleInfoFallbackProc)(
        PFORT_MODULE_IMP moduleImp, PLOADEDMODULE pModule, LPCSTR name);

typedef FARPROC (*ModuleGetProcAddressProc)(PLOADEDMODULE pModule, LPCSTR funcName);

typedef FARPROC (*ModuleGetProcAddressFallbackProc)(
        PFORT_MODULE_IMP moduleImp, PLOADEDMODULE pModule, LPCSTR funcName);

typedef NTSTATUS (*BuildImportTableEntryBeginProc)(
        PFORT_MODULE_IMP moduleImp, PLOADEDMODULE pModule, PIMAGE_NT_HEADERS pHeaders);

typedef void (*BuildImportTableEntryEndProc)(PFORT_MODULE_IMP moduleImp, NTSTATUS status);

typedef void (*BuildImportTableLibraryBeginProc)(PFORT_MODULE_IMP moduleImp, LPCSTR libName);

#define FORT_MODULE_IMP_DATA_SIZE 16

typedef struct fort_module_imp_ext
{
    PVOID reserved;
} FORT_MODULE_IMP_EXT, *PFORT_MODULE_IMP_EXT;

typedef struct fort_module_imp
{
    DWORD osMajorVersion;
    DWORD osMinorVersion;
    DWORD osBuildNumber;
    DWORD osPlatformId;

    DWORD modulesCount;
    PAUX_MODULE_EXTENDED_INFO modules;

    DbgPrintExProc dbgPrintEx;
    StrICmpProc strICmp;

    GetModuleInfoProc getModuleInfo;
    GetModuleInfoFallbackProc getModuleInfoFallback;
    ModuleGetProcAddressProc moduleGetProcAddress;
    ModuleGetProcAddressFallbackProc moduleGetProcAddressFallback;

    BuildImportTableEntryBeginProc buildImportTableEntriesBegin;
    BuildImportTableEntryEndProc buildImportTableEntriesEnd;

    BuildImportTableLibraryBeginProc buildImportTableLibraryBegin;
    PLOADEDMODULE forwardModule;

    LOADEDMODULE kernelModule;

    union {
        FORT_MODULE_IMP_EXT ext;

        PVOID data[FORT_MODULE_IMP_DATA_SIZE];
    };
} FORT_MODULE_IMP;

static_assert(sizeof(FORT_MODULE_IMP_EXT) <= FORT_MODULE_IMP_DATA_SIZE * sizeof(PVOID),
        "FORT_MODULE_IMP_EXT size mismatch");

FORT_API void InitModuleImporter(
        PFORT_MODULE_IMP moduleImp, const PAUX_MODULE_EXTENDED_INFO modules, DWORD modulesCount);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTMM_IMP_H
