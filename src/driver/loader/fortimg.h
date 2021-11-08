#ifndef FORTIMG_H
#define FORTIMG_H

#include "fortmm.h"

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API NTSTATUS fort_image_load(const PUCHAR data, DWORD dataSize, LOADEDMODULE *module);
FORT_API VOID fort_image_unload(LOADEDMODULE *module);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTIMG_H
