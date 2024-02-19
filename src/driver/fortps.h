#ifndef FORTPS_H
#define FORTPS_H

#include "fortdrv.h"

#include "fortcnf.h"
#include "fortpool.h"
#include "forttds.h"

#define FORT_PSTREE_ACTIVE 0x0001

typedef struct fort_pstree
{
    UCHAR volatile flags;

    UINT16 procs_n;

    FORT_POOL_LIST pool_list;
    tommy_list free_procs;

    tommy_arrayof procs;
    tommy_hashdyn procs_map;

    KSPIN_LOCK lock;
} FORT_PSTREE, *PFORT_PSTREE;

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API void fort_pstree_open(PFORT_PSTREE ps_tree);

FORT_API void fort_pstree_close(PFORT_PSTREE ps_tree);

FORT_API void fort_pstree_enum_processes(PFORT_PSTREE ps_tree);

FORT_API BOOL fort_pstree_get_proc_name(PFORT_PSTREE ps_tree, DWORD processId, PUNICODE_STRING path,
        BOOL *isSvcHost, BOOL *inherited);

FORT_API void fort_pstree_update_services(
        PFORT_PSTREE ps_tree, const PFORT_SERVICE_INFO_LIST services, ULONG data_len);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTPS_H
