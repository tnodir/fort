#ifndef FORTPS_H
#define FORTPS_H

#include "fortdrv.h"

typedef struct fort_pstree
{
    INT16 head_index;
    INT16 free_index;
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
