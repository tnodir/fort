#include "um_ndis.h"

NDIS_STATUS NdisRetreatNetBufferDataStart(PNET_BUFFER netBuffer, ULONG dataOffsetDelta,
        ULONG dataBackFill, NET_BUFFER_ALLOCATE_MDL_HANDLER allocateMdlHandler)
{
    UNUSED(netBuffer);
    UNUSED(dataOffsetDelta);
    UNUSED(dataBackFill);
    UNUSED(allocateMdlHandler);
    return 0;
}

void NdisAdvanceNetBufferDataStart(PNET_BUFFER netBuffer, ULONG dataOffsetDelta, BOOLEAN freeMdl,
        NET_BUFFER_FREE_MDL_HANDLER freeMdlHandler)
{
    UNUSED(netBuffer);
    UNUSED(dataOffsetDelta);
    UNUSED(freeMdl);
    UNUSED(freeMdlHandler);
}

PVOID NdisGetDataBuffer(PNET_BUFFER netBuffer, ULONG bytesNeeded, PVOID storage, UINT alignMultiple,
        UINT alignOffset)
{
    UNUSED(netBuffer);
    UNUSED(bytesNeeded);
    UNUSED(storage);
    UNUSED(alignMultiple);
    UNUSED(alignOffset);
    return NULL;
}
