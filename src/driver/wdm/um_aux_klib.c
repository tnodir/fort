#include "um_aux_klib.h"

NTSTATUS AuxKlibInitialize()
{
    return STATUS_SUCCESS;
}

NTSTATUS AuxKlibQueryModuleInformation(PULONG BufferSize, ULONG ElementSize, PVOID QueryInfo)
{
    UNUSED(BufferSize);
    UNUSED(ElementSize);
    UNUSED(QueryInfo);
    return STATUS_SUCCESS;
}
