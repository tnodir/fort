#ifndef UM_NTIFS_H
#define UM_NTIFS_H

#include "um_wdm.h"

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API NTSTATUS RtlDowncaseUnicodeString(PUNICODE_STRING destinationString,
        PCUNICODE_STRING sourceString, BOOLEAN allocateDestinationString);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // UM_NTIFS_H
