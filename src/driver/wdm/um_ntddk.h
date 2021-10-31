#ifndef UM_NTDDK_H
#define UM_NTDDK_H

#include "um_wdm.h"

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API NTSTATUS IoQueryFullDriverPath(
        _In_ PDRIVER_OBJECT DriverObject, _Out_ PUNICODE_STRING FullPath);

//
// Define driver reinitialization routine type.
//

typedef VOID DRIVER_REINITIALIZE(PDRIVER_OBJECT DriverObject, PVOID Context, ULONG Count);

typedef DRIVER_REINITIALIZE *PDRIVER_REINITIALIZE;

FORT_API VOID IoRegisterDriverReinitialization(_In_ PDRIVER_OBJECT DriverObject,
        _In_ PDRIVER_REINITIALIZE DriverReinitializationRoutine, _In_opt_ PVOID Context);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // UM_NTDDK_H
