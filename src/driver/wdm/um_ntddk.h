#ifndef UM_NTDDK_H
#define UM_NTDDK_H

#include "um_wdm.h"

#if defined(__cplusplus)
extern "C" {
#endif

//
// Define driver reinitialization routine type.
//

typedef VOID DRIVER_REINITIALIZE(PDRIVER_OBJECT DriverObject, PVOID Context, ULONG Count);

typedef DRIVER_REINITIALIZE *PDRIVER_REINITIALIZE;

FORT_API VOID IoRegisterDriverReinitialization(PDRIVER_OBJECT DriverObject,
        PDRIVER_REINITIALIZE DriverReinitializationRoutine, PVOID Context);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // UM_NTDDK_H
