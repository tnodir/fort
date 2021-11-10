#ifndef UM_AUX_KLIB_H
#define UM_AUX_KLIB_H

#include "um_wdm.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define AUX_KLIB_MODULE_PATH_LEN 256

typedef struct _AUX_MODULE_BASIC_INFO
{
    PVOID ImageBase;
} AUX_MODULE_BASIC_INFO, *PAUX_MODULE_BASIC_INFO;

typedef struct _AUX_MODULE_EXTENDED_INFO
{
    AUX_MODULE_BASIC_INFO BasicInfo;
    ULONG ImageSize;
    USHORT FileNameOffset;
    UCHAR FullPathName[AUX_KLIB_MODULE_PATH_LEN];
} AUX_MODULE_EXTENDED_INFO, *PAUX_MODULE_EXTENDED_INFO;

FORT_API NTSTATUS AuxKlibInitialize(VOID);

FORT_API NTSTATUS AuxKlibQueryModuleInformation(
        PULONG BufferSize, ULONG ElementSize, PVOID QueryInfo);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // UM_AUX_KLIB_H
