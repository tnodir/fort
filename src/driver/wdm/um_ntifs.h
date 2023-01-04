#ifndef UM_NTIFS_H
#define UM_NTIFS_H

#include "um_wdm.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct _KAPC_STATE
{
    LIST_ENTRY ApcListHead[MaximumMode];
    struct _KPROCESS *Process;
    union {
        UCHAR InProgressFlags;
        struct
        {
            BOOLEAN KernelApcInProgress : 1;
            BOOLEAN SpecialApcInProgress : 1;
        };
    };

    BOOLEAN KernelApcPending;
    union {
        BOOLEAN UserApcPendingAll;
        struct
        {
            BOOLEAN SpecialUserApcPending : 1;
            BOOLEAN UserApcPending : 1;
        };
    };
} KAPC_STATE, *PKAPC_STATE, *PRKAPC_STATE;

FORT_API NTSTATUS RtlDowncaseUnicodeString(PUNICODE_STRING destinationString,
        PCUNICODE_STRING sourceString, BOOLEAN allocateDestinationString);

FORT_API NTSTATUS PsLookupProcessByProcessId(HANDLE processId, PEPROCESS *process);

FORT_API NTSTATUS ObOpenObjectByPointer(PVOID object, ULONG handleAttributes,
        PACCESS_STATE passedAccessState, ACCESS_MASK desiredAccess, POBJECT_TYPE objectType,
        KPROCESSOR_MODE accessMode, PHANDLE handle);

FORT_API VOID KeStackAttachProcess(PRKPROCESS process, PRKAPC_STATE apcState);

FORT_API VOID KeUnstackDetachProcess(PRKAPC_STATE apcState);

FORT_API ULONG RtlRandomEx(PULONG seed);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // UM_NTIFS_H
