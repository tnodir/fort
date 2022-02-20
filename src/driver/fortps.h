#ifndef FORTPS_H
#define FORTPS_H

#include "fortdrv.h"

#include "fortcnf.h"
#include "fortpool.h"
#include "forttds.h"

typedef struct fort_pstree
{
    UINT8 active : 1;

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

FORT_API void NTAPI fort_pstree_enum_processes(void);

FORT_API BOOL fort_pstree_get_proc_name(PFORT_PSTREE ps_tree, PFORT_CONF_REF conf_ref,
        DWORD processId, PUNICODE_STRING path, BOOL *inherited);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTPS_H
