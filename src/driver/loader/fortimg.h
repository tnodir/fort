#ifndef FORTIMG_H
#define FORTIMG_H

#include "fortdl.h"

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API NTSTATUS fort_image_payload(
        const PUCHAR data, DWORD dataSize, PUCHAR *outPayload, DWORD *outPayloadSize);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTIMG_H
