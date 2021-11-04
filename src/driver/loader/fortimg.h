#ifndef FORTIMG_H
#define FORTIMG_H

#include "fortdl.h"

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API NTSTATUS fort_image_load(const PUCHAR data, DWORD dataSize, PUCHAR *image, DWORD *outSize);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTIMG_H
