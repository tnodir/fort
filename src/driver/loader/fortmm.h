#ifndef FORTMM_H
#define FORTMM_H

#include "fortdl.h"

#include "../proxycb/fortpcb_def.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct _LOADEDMODULE
{
    PUCHAR codeBase;
} LOADEDMODULE, *PLOADEDMODULE;

FORT_API NTSTATUS LoadModuleFromMemory(PLOADEDMODULE pModule, PUCHAR lpData, DWORD dwSize);

FORT_API void UnloadModule(PLOADEDMODULE pModule);

FORT_API NTSTATUS SetupModuleCallbacks(PLOADEDMODULE pModule, PFORT_PROXYCB_INFO cbInfo);

FORT_API NTSTATUS CallModuleEntry(
        PLOADEDMODULE pModule, PDRIVER_OBJECT driver, PUNICODE_STRING regPath);

FORT_API FARPROC ModuleGetProcAddress(PLOADEDMODULE pModule, LPCSTR funcName);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTMM_H
