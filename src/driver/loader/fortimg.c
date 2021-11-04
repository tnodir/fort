/* Fort Firewall Driver Image Handling */

#include "fortimg.h"

FORT_API NTSTATUS fort_image_load(const PUCHAR data, DWORD dataSize, PUCHAR *image, DWORD *outSize)
{
    NTSTATUS status;

    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "FORT: Loader Load Image: %d\n", dataSize);

    return STATUS_SUCCESS;
}
