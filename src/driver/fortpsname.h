#ifndef FORTPSNAME_H
#define FORTPSNAME_H

#include "fortdrv.h"

#include "fortpool.h"
#include "forttds.h"

/* The process's name, shared by its users: the equal names are the same one */
typedef struct fort_psname_node
{
    tommy_hashdyn_node node;

    LONG volatile refcount;

    UINT16 size;

    WCHAR data[1]; /* zero-terminated */
} FORT_PSNAME_NODE, *PFORT_PSNAME_NODE;

typedef const FORT_PSNAME_NODE *PCFORT_PSNAME_NODE;

typedef struct fort_psnames
{
    FORT_POOL_LIST pool_list;

    tommy_hashdyn names_map;

    KSPIN_LOCK lock;
} FORT_PSNAMES, *PFORT_PSNAMES;

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API void fort_psname_open(PFORT_PSNAMES ps_names);

FORT_API void fort_psname_close(PFORT_PSNAMES ps_names);

/* Returns the path's name, which is referenced until fort_psname_put() */
FORT_API PFORT_PSNAME_NODE fort_psname_get(PFORT_PSNAMES ps_names, PCFORT_APP_PATH path);

/* References the name held by the caller once more */
FORT_API void fort_psname_ref(PFORT_PSNAME_NODE ps_name);

FORT_API void fort_psname_put(PFORT_PSNAMES ps_names, PFORT_PSNAME_NODE ps_name);

FORT_API BOOL fort_psname_equal(PCFORT_PSNAME_NODE ps_name, PCFORT_APP_PATH path);

FORT_API FORT_APP_PATH fort_psname_path(PCFORT_PSNAME_NODE ps_name);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTPSNAME_H
