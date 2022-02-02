#ifndef FORTPS_H
#define FORTPS_H

#include "fortdrv.h"

#include "fortpool.h"
#include "forttds.h"

typedef struct fort_pstree
{
    UINT16 procs_n;

    FORT_POOL_LIST pool_list;
    tommy_list free_procs;

    tommy_arrayof procs;
    tommy_hashdyn procs_map;

    KSPIN_LOCK lock;
} FORT_PSTREE, *PFORT_PSTREE;

typedef struct fort_psname
{
    UINT8 refcount;
    UINT8 size;
    WCHAR data[1];
} FORT_PSNAME, *PFORT_PSNAME;

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API void fort_pstree_open(PFORT_PSTREE ps_tree);

FORT_API void fort_pstree_close(PFORT_PSTREE ps_tree);

FORT_API PFORT_PSNAME fort_pstree_acquire_proc_name(PFORT_PSTREE ps_tree, DWORD processId);

FORT_API void fort_pstree_release_proc_name(PFORT_PSTREE ps_tree, PFORT_PSNAME ps_name);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTPS_H
