#include "um_ntddk.h"

VOID IoRegisterDriverReinitialization(PDRIVER_OBJECT DriverObject,
        PDRIVER_REINITIALIZE DriverReinitializationRoutine, PVOID Context)
{
    UNUSED(DriverObject);
    UNUSED(DriverReinitializationRoutine);
    UNUSED(Context);
}
