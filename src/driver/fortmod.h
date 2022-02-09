#ifndef FORTMOD_H
#define FORTMOD_H

#include "fortdrv.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct _AUX_MODULE_EXTENDED_INFO AUX_MODULE_EXTENDED_INFO, *PAUX_MODULE_EXTENDED_INFO;

typedef struct _LOADEDMODULE
{
    PUCHAR codeBase;
} LOADEDMODULE, *PLOADEDMODULE;

FORT_API PIMAGE_NT_HEADERS GetModuleNtHeaders(PUCHAR codeBase);

FORT_API NTSTATUS GetModuleInfo(PLOADEDMODULE pModule, LPCSTR name,
        const PAUX_MODULE_EXTENDED_INFO modules, DWORD modulesCount);

FORT_API NTSTATUS GetModuleInfoList(PAUX_MODULE_EXTENDED_INFO *outModules, DWORD *outModulesCount);

FORT_API void FreeModuleInfoList(PAUX_MODULE_EXTENDED_INFO modules);

FORT_API FARPROC ModuleGetProcAddress(PLOADEDMODULE pModule, LPCSTR funcName);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTMOD_H
