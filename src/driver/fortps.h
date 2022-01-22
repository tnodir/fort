#ifndef FORTPS_H
#define FORTPS_H

#include "fortdrv.h"

#include "fortpool.h"
#include "forttds.h"

typedef struct fort_pstree
{
    FORT_POOL_LIST pool_list;
    tommy_list free_nodes;

    tommy_arrayof procs;
    tommy_hashdyn procs_map;

    KSPIN_LOCK lock;
} FORT_PSTREE, *PFORT_PSTREE;

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API void fort_pstree_open(PFORT_PSTREE ps_tree);

FORT_API void fort_pstree_close(PFORT_PSTREE ps_tree);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTPS_H
