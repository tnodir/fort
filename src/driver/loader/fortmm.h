#ifndef FORTMM_H
#define FORTMM_H

#include "fortdl.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct _LOADEDMODULE
{
    PIMAGE_NT_HEADERS pHeaders;
    PBYTE pCodeBase;
    HINSTANCE *pModules;
    INT nModules;
    BOOL fInitialized;
    BOOL fRelocated;
    DWORD dwPageSize;
} LOADEDMODULE, *PLOADEDMODULE;

FORT_API NTSTATUS LoadModuleFromMemory(
        __in PLOADEDMODULE pModule, __in_bcount(dwSize) PVOID lpData, __in DWORD dwSize);
FORT_API void UnloadModule(__in PLOADEDMODULE pModule);

FORT_API FARPROC ModuleGetProcAddress(__in PLOADEDMODULE pModule, __in_z_opt LPCSTR FuncName);

FORT_API PVOID ModuleFindResource(
        __in PLOADEDMODULE module, __in_z LPCTSTR name, __in_z_opt LPCTSTR type);
FORT_API DWORD ModuleSizeofResource(__in PLOADEDMODULE module, __in_opt PVOID resource);
FORT_API LPVOID ModuleLoadResource(__in PLOADEDMODULE module, __in PVOID resource);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTMM_H
