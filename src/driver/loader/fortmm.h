#ifndef FORTMM_H
#define FORTMM_H

#include "fortdl.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct _LOADEDMODULE
{
    PUCHAR codeBase;
} LOADEDMODULE, *PLOADEDMODULE;

FORT_API NTSTATUS LoadModuleFromMemory(
        __in PLOADEDMODULE pModule, __in_bcount(dwSize) PVOID lpData, __in DWORD dwSize);

FORT_API NTSTATUS CallModuleEntry(
        __in PLOADEDMODULE pModule, __in PDRIVER_OBJECT driver, __in PUNICODE_STRING regPath);

FORT_API void UnloadModule(__in PLOADEDMODULE pModule);

FORT_API FARPROC ModuleGetProcAddress(__in PLOADEDMODULE pModule, __in_z_opt LPCSTR funcName);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTMM_H
