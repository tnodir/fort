#ifndef UM_NDIS_H
#define UM_NDIS_H

#include "um_wdm.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef PVOID MDL, *PMDL;

typedef PVOID NDIS_HANDLE, *PNDIS_HANDLE;
typedef int NDIS_STATUS, *PNDIS_STATUS;

typedef PHYSICAL_ADDRESS NDIS_PHYSICAL_ADDRESS, *PNDIS_PHYSICAL_ADDRESS;

//
// NET_BUFFER data structures, APIs and macros
//

typedef struct _NET_BUFFER NET_BUFFER, *PNET_BUFFER;
typedef struct _NET_BUFFER_LIST_CONTEXT NET_BUFFER_LIST_CONTEXT, *PNET_BUFFER_LIST_CONTEXT;
typedef struct _NET_BUFFER_LIST NET_BUFFER_LIST, *PNET_BUFFER_LIST;

struct _SCATTER_GATHER_LIST;
typedef struct _SCATTER_GATHER_LIST SCATTER_GATHER_LIST, *PSCATTER_GATHER_LIST;

typedef union _NET_BUFFER_DATA_LENGTH {
    ULONG DataLength;
    SIZE_T stDataLength;
} NET_BUFFER_DATA_LENGTH, *PNET_BUFFER_DATA_LENGTH;

typedef struct _NET_BUFFER_DATA
{
    PNET_BUFFER Next;
    PMDL CurrentMdl;
    ULONG CurrentMdlOffset;
#ifdef __cplusplus
    NET_BUFFER_DATA_LENGTH NbDataLength;
#else
    NET_BUFFER_DATA_LENGTH;
#endif
    PMDL MdlChain;
    ULONG DataOffset;
} NET_BUFFER_DATA, *PNET_BUFFER_DATA;

typedef union _NET_BUFFER_HEADER {
#ifdef __cplusplus
    NET_BUFFER_DATA NetBufferData;
#else
    NET_BUFFER_DATA;
#endif
    SLIST_HEADER Link;

} NET_BUFFER_HEADER, *PNET_BUFFER_HEADER;

typedef struct _NET_BUFFER
{
    union {
        struct
        {
            PNET_BUFFER Next;
            PMDL CurrentMdl;
            ULONG CurrentMdlOffset;
            union {
                ULONG DataLength;
                SIZE_T stDataLength;
            };

            PMDL MdlChain;
            ULONG DataOffset;
        };

        SLIST_HEADER Link;

        // Duplicate of the above union, for source-compatibility
        NET_BUFFER_HEADER NetBufferHeader;
    };

    USHORT ChecksumBias;
    USHORT Reserved;
    NDIS_HANDLE NdisPoolHandle;
    DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) PVOID NdisReserved[2];
    DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) PVOID ProtocolReserved[6];
    DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) PVOID MiniportReserved[4];
    NDIS_PHYSICAL_ADDRESS DataPhysicalAddress;
} NET_BUFFER, *PNET_BUFFER;

#pragma warning(push)
#pragma warning(disable : 4200) // nonstandard extension used : zero-sized array in struct/union

typedef struct _NET_BUFFER_LIST_CONTEXT
{
    PNET_BUFFER_LIST_CONTEXT Next;
    USHORT Size;
    USHORT Offset;
    DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) UCHAR ContextData[];
} NET_BUFFER_LIST_CONTEXT, *PNET_BUFFER_LIST_CONTEXT;

#pragma warning(pop)

typedef enum _NDIS_NET_BUFFER_LIST_INFO {
    TcpIpChecksumNetBufferListInfo,
    TcpOffloadBytesTransferred = TcpIpChecksumNetBufferListInfo,
    IPsecOffloadV1NetBufferListInfo,
    TcpLargeSendNetBufferListInfo,
    TcpReceiveNoPush = TcpLargeSendNetBufferListInfo,
    ClassificationHandleNetBufferListInfo,
    Ieee8021QNetBufferListInfo,
    NetBufferListCancelId,
    MediaSpecificInformation,
    NetBufferListFrameType,
    NetBufferListProtocolId = NetBufferListFrameType,
    NetBufferListHashValue,
    NetBufferListHashInfo,
    WfpNetBufferListInfo,

    MaxNetBufferListInfo
} NDIS_NET_BUFFER_LIST_INFO,
        *PNDIS_NET_BUFFER_LIST_INFO;

typedef struct _NET_BUFFER_LIST_DATA
{
    PNET_BUFFER_LIST Next; // Next NetBufferList in the chain
    PNET_BUFFER FirstNetBuffer; // First NetBuffer on this NetBufferList
} NET_BUFFER_LIST_DATA, *PNET_BUFFER_LIST_DATA;

typedef union _NET_BUFFER_LIST_HEADER {
#ifdef __cplusplus
    NET_BUFFER_LIST_DATA NetBufferListData;
#else
    NET_BUFFER_LIST_DATA;
#endif
    SLIST_HEADER Link; // used in SLIST of free NetBuffers in the block
} NET_BUFFER_LIST_HEADER, *PNET_BUFFER_LIST_HEADER;

typedef struct _NET_BUFFER_LIST
{
    union {
        struct
        {
            PNET_BUFFER_LIST Next; // Next NetBufferList in the chain
            PNET_BUFFER FirstNetBuffer; // First NetBuffer on this NetBufferList
        };

        SLIST_HEADER Link; // used in SLIST of free NetBuffers in the block

        // Duplicate of the above, for source-compatibility
        NET_BUFFER_LIST_HEADER NetBufferListHeader;
    };

    PNET_BUFFER_LIST_CONTEXT Context;
    PNET_BUFFER_LIST ParentNetBufferList;
    NDIS_HANDLE NdisPoolHandle;
    DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) PVOID NdisReserved[2];
    DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) PVOID ProtocolReserved[4];
    DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) PVOID MiniportReserved[2];
    PVOID Scratch;
    NDIS_HANDLE SourceHandle;
    ULONG NblFlags; // public flags
    LONG ChildRefCount;
    ULONG Flags; // private flags used by NDIs, protocols, miniport, etc.

    union {
        NDIS_STATUS Status;
        ULONG NdisReserved2;
    };

    PVOID NetBufferListInfo[MaxNetBufferListInfo];
} NET_BUFFER_LIST, *PNET_BUFFER_LIST;

#define NET_BUFFER_NEXT_NB(_NB)            ((_NB)->Next)
#define NET_BUFFER_FIRST_MDL(_NB)          ((_NB)->MdlChain)
#define NET_BUFFER_DATA_LENGTH(_NB)        ((_NB)->DataLength)
#define NET_BUFFER_DATA_OFFSET(_NB)        ((_NB)->DataOffset)
#define NET_BUFFER_CURRENT_MDL(_NB)        ((_NB)->CurrentMdl)
#define NET_BUFFER_CURRENT_MDL_OFFSET(_NB) ((_NB)->CurrentMdlOffset)

#define NET_BUFFER_PROTOCOL_RESERVED(_NB) ((_NB)->ProtocolReserved)
#define NET_BUFFER_MINIPORT_RESERVED(_NB) ((_NB)->MiniportReserved)
#define NET_BUFFER_CHECKSUM_BIAS(_NB)     ((_NB)->ChecksumBias)

#if (NDIS_SUPPORT_NDIS61)
#    define NET_BUFFER_DATA_PHYSICAL_ADDRESS(_NB) ((_NB)->DataPhysicalAddress)
#endif // (NDIS_SUPPORT_NDIS61)

#if (NDIS_SUPPORT_NDIS620)
#    define NET_BUFFER_FIRST_SHARED_MEM_INFO(_NB)    ((_NB)->SharedMemoryInfo)
#    define NET_BUFFER_SHARED_MEM_NEXT_SEGMENT(_SHI) ((_SHI)->NextSharedMemorySegment)
#    define NET_BUFFER_SHARED_MEM_FLAGS(_SHI)        ((_SHI)->SharedMemoryFlags)
#    define NET_BUFFER_SHARED_MEM_HANDLE(_SHI)       ((_SHI)->SharedMemoryHandle)
#    define NET_BUFFER_SHARED_MEM_OFFSET(_SHI)       ((_SHI)->SharedMemoryOffset)
#    define NET_BUFFER_SHARED_MEM_LENGTH(_SHI)       ((_SHI)->SharedMemoryLength)

#    define NET_BUFFER_SCATTER_GATHER_LIST(_NB) ((_NB)->ScatterGatherList)

#endif // (NDIS_SUPPORT_NDIS620)

#define NET_BUFFER_LIST_NEXT_NBL(_NBL) ((_NBL)->Next)
#define NET_BUFFER_LIST_FIRST_NB(_NBL) ((_NBL)->FirstNetBuffer)

#define NET_BUFFER_LIST_FLAGS(_NBL)             ((_NBL)->Flags)
#define NET_BUFFER_LIST_NBL_FLAGS(_NBL)         ((_NBL)->NblFlags)
#define NET_BUFFER_LIST_PROTOCOL_RESERVED(_NBL) ((_NBL)->ProtocolReserved)
#define NET_BUFFER_LIST_MINIPORT_RESERVED(_NBL) ((_NBL)->MiniportReserved)
#define NET_BUFFER_LIST_CONTEXT_DATA_START(_NBL)                                                   \
    ((PUCHAR)(((_NBL)->Context) + 1) + (_NBL)->Context->Offset)
#define NET_BUFFER_LIST_CONTEXT_DATA_SIZE(_NBL) (((_NBL)->Context)->Size)

#define NET_BUFFER_LIST_INFO(_NBL, _Id) ((_NBL)->NetBufferListInfo[(_Id)])
#define NET_BUFFER_LIST_STATUS(_NBL)    ((_NBL)->Status)

#define NDIS_GET_NET_BUFFER_LIST_CANCEL_ID(_NBL) (NET_BUFFER_LIST_INFO(_NBL, NetBufferListCancelId))
#define NDIS_SET_NET_BUFFER_LIST_CANCEL_ID(_NBL, _CancelId)                                        \
    NET_BUFFER_LIST_INFO(_NBL, NetBufferListCancelId) = _CancelId

#define NDIS_GET_NET_BUFFER_LIST_IM_RESERVED(_NBL) (NET_BUFFER_LIST_INFO(_NBL, IMReserved))
#define NDIS_SET_NET_BUFFER_LIST_IM_RESERVED(_NBL, _Val)                                           \
    NET_BUFFER_LIST_INFO(_NBL, IMReserved) = (_Val)

#define NDIS_GET_NET_BUFFER_LIST_VLAN_ID(_NBL)                                                     \
    (((NDIS_NET_BUFFER_LIST_8021Q_INFO *) &NET_BUFFER_LIST_INFO(                                   \
              (_NBL), Ieee8021QNetBufferListInfo))                                                 \
                    ->TagHeader.VlanId)
#define NDIS_SET_NET_BUFFER_LIST_VLAN_ID(_NBL, _VlanId)                                            \
    ((NDIS_NET_BUFFER_LIST_8021Q_INFO *) &NET_BUFFER_LIST_INFO(                                    \
             (_NBL), Ieee8021QNetBufferListInfo))                                                  \
            ->TagHeader.VlanId = (_VlanId)
//
//  Per-NBL information for Ieee8021QNetBufferListInfo.
//
typedef struct _NDIS_NET_BUFFER_LIST_8021Q_INFO
{
    union {
        struct
        {
            UINT32 UserPriority : 3; // 802.1p priority
            UINT32 CanonicalFormatId : 1; // always 0
            UINT32 VlanId : 12; // VLAN Identification
            UINT32 Reserved : 16; // set to 0 for ethernet
        } TagHeader;

        struct
        {
            UINT32 UserPriority : 3; // 802.1p priority
            UINT32 CanonicalFormatId : 1; // always 0
            UINT32 VlanId : 12; // VLAN Identification
            UINT32 WMMInfo : 4;
            UINT32 Reserved : 12; // set to 0 for wireless lan

        } WLanTagHeader;

        PVOID Value;
    };
} NDIS_NET_BUFFER_LIST_8021Q_INFO, *PNDIS_NET_BUFFER_LIST_8021Q_INFO;

typedef struct _NDIS_NET_BUFFER_LIST_MEDIA_SPECIFIC_INFO
{
    union {
        PVOID MediaSpecificInfo;
        PVOID NativeWifiSpecificInfo;

        PVOID Value;
    };

} NDIS_NET_BUFFER_LIST_MEDIA_SPECIFIC_INFO, *PNDIS_NET_BUFFER_LIST_MEDIA_SPECIFIC_INFO;

typedef struct _NDIS_NBL_MEDIA_MEDIA_SPECIFIC_INFORMATION NDIS_NBL_MEDIA_SPECIFIC_INFORMATION,
        *PNDIS_NBL_MEDIA_SPECIFIC_INFORMATION;

struct _NDIS_NBL_MEDIA_MEDIA_SPECIFIC_INFORMATION
{
    PNDIS_NBL_MEDIA_SPECIFIC_INFORMATION NextEntry;
    ULONG Tag;
    DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) UCHAR Data[1];
};

#define NDIS_NBL_ADD_MEDIA_SPECIFIC_INFO(_NBL, _MediaSpecificInfo)                                 \
    {                                                                                              \
        PNDIS_NBL_MEDIA_SPECIFIC_INFORMATION HeadEntry = NULL;                                     \
        if (NET_BUFFER_LIST_INFO((_NBL), MediaSpecificInformation) != NULL) {                      \
            HeadEntry = (PNDIS_NBL_MEDIA_SPECIFIC_INFORMATION)(                                    \
                    NET_BUFFER_LIST_INFO((_NBL), MediaSpecificInformation));                       \
        }                                                                                          \
        NET_BUFFER_LIST_INFO((_NBL), MediaSpecificInformation) = (_MediaSpecificInfo);             \
        (_MediaSpecificInfo)->NextEntry = HeadEntry;                                               \
    }

#define NDIS_NBL_REMOVE_MEDIA_SPECIFIC_INFO(_NBL, _MediaSpecificInfo)                              \
    {                                                                                              \
        PNDIS_NBL_MEDIA_SPECIFIC_INFORMATION *HeadEntry;                                           \
        HeadEntry = (PNDIS_NBL_MEDIA_SPECIFIC_INFORMATION *) &(                                    \
                NET_BUFFER_LIST_INFO((_NBL), MediaSpecificInformation));                           \
        for (; *HeadEntry != NULL; HeadEntry = &(*HeadEntry)->NextEntry) {                         \
            if ((*HeadEntry)->Tag == (_MediaSpecificInfo)->Tag) {                                  \
                *HeadEntry = (*HeadEntry)->NextEntry;                                              \
                break;                                                                             \
            }                                                                                      \
        }                                                                                          \
    }

#define NDIS_NBL_GET_MEDIA_SPECIFIC_INFO(_NBL, _Tag, _MediaSpecificInfo)                           \
    {                                                                                              \
        PNDIS_NBL_MEDIA_SPECIFIC_INFORMATION HeadEntry;                                            \
        (_MediaSpecificInfo) = NULL;                                                               \
        HeadEntry = (PNDIS_NBL_MEDIA_SPECIFIC_INFORMATION)(                                        \
                NET_BUFFER_LIST_INFO((_NBL), MediaSpecificInformation));                           \
        for (; HeadEntry != NULL; HeadEntry = HeadEntry->NextEntry) {                              \
            if (HeadEntry->Tag == (_Tag)) {                                                        \
                (_MediaSpecificInfo) = HeadEntry;                                                  \
                break;                                                                             \
            }                                                                                      \
        }                                                                                          \
    }

/*
Bit  31 - 0 for MS tag
          1 for Vendor tag
Bits 30-16 - Vendor ID (if Bit 31 = 1)
           - Technology ID (if Bit 31 = 0)
Bits 15-0 - Tag ID

*/

//
// Microsoft Media Specific-Info tags
//
// TUNNEL - Technology ID : 1
#define NDIS_MEDIA_SPECIFIC_INFO_TUNDL 0x00010001
//
// Intel Media Specific Info tags
//
#define NDIS_MEDIA_SPECIFIC_INFO_FCOE     0x80010000
#define NDIS_MEDIA_SPECIFIC_INFO_EAPOL    0x80010001
#define NDIS_MEDIA_SPECIFIC_INFO_LLDP     0x80010002
#define NDIS_MEDIA_SPECIFIC_INFO_TIMESYNC 0x80010003

#ifndef NDIS_HASH_FUNCTION_MASK
#    define NDIS_HASH_FUNCTION_MASK 0x000000FF
#    define NDIS_HASH_TYPE_MASK     0x00FFFF00
#endif

//
// The following macros are used by miniport driver and protocol driver to set and get
// the hash value, hash type and hash function.
//
VOID FORCEINLINE NET_BUFFER_LIST_SET_HASH_TYPE(
        _In_ PNET_BUFFER_LIST _NBL, _In_ volatile ULONG _HashType)
{
    (NET_BUFFER_LIST_INFO(_NBL, NetBufferListHashInfo) =
                    UlongToPtr(((PtrToUlong(NET_BUFFER_LIST_INFO(_NBL, NetBufferListHashInfo))
                                        & (~NDIS_HASH_TYPE_MASK))
                            | ((_HashType) & (NDIS_HASH_TYPE_MASK)))));
}

VOID FORCEINLINE NET_BUFFER_LIST_SET_HASH_FUNCTION(
        _In_ PNET_BUFFER_LIST _NBL, _In_ volatile ULONG _HashFunction)
{
    (NET_BUFFER_LIST_INFO(_NBL, NetBufferListHashInfo) =
                    UlongToPtr(((PtrToUlong(NET_BUFFER_LIST_INFO(_NBL, NetBufferListHashInfo))
                                        & (~NDIS_HASH_FUNCTION_MASK))
                            | ((_HashFunction) & (NDIS_HASH_FUNCTION_MASK)))));
}

#define NET_BUFFER_LIST_SET_HASH_VALUE(_NBL, _HashValue)                                           \
    (NET_BUFFER_LIST_INFO(_NBL, NetBufferListHashValue) = UlongToPtr(_HashValue))

#define NET_BUFFER_LIST_GET_HASH_TYPE(_NBL)                                                        \
    (PtrToUlong(NET_BUFFER_LIST_INFO(_NBL, NetBufferListHashInfo)) & (NDIS_HASH_TYPE_MASK))

#define NET_BUFFER_LIST_GET_HASH_FUNCTION(_NBL)                                                    \
    (PtrToUlong(NET_BUFFER_LIST_INFO(_NBL, NetBufferListHashInfo)) & (NDIS_HASH_FUNCTION_MASK))

#define NET_BUFFER_LIST_GET_HASH_VALUE(_NBL)                                                       \
    (PtrToUlong(NET_BUFFER_LIST_INFO(_NBL, NetBufferListHashValue)))

#define NdisSetNetBufferListProtocolId(_NBL, _ProtocolId)                                          \
    *((PUCHAR)(&NET_BUFFER_LIST_INFO(_NBL, NetBufferListProtocolId))) = _ProtocolId

//
// Prototypes of the MDL allocation and free callback.
//

typedef PMDL (*NET_BUFFER_ALLOCATE_MDL_HANDLER)(PULONG BufferSize);
typedef VOID (*NET_BUFFER_FREE_MDL_HANDLER)(PMDL Mdl);

FORT_API NDIS_STATUS NdisRetreatNetBufferDataStart(PNET_BUFFER netBuffer, ULONG dataOffsetDelta,
        ULONG dataBackFill, NET_BUFFER_ALLOCATE_MDL_HANDLER allocateMdlHandler);

FORT_API VOID NdisAdvanceNetBufferDataStart(PNET_BUFFER netBuffer, ULONG dataOffsetDelta,
        BOOLEAN freeMdl, NET_BUFFER_FREE_MDL_HANDLER freeMdlHandler);

FORT_API PVOID NdisGetDataBuffer(PNET_BUFFER netBuffer, ULONG bytesNeeded, PVOID storage,
        UINT alignMultiple, UINT alignOffset);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // UM_NDIS_H
