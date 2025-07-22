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

#define FORT_PSNODE_NAME_INHERIT      0x0001
#define FORT_PSNODE_NAME_INHERIT_SPEC 0x0002
#define FORT_PSNODE_NAME_INHERITED    0x0004
#define FORT_PSNODE_NAME_CUSTOM       0x0008
#define FORT_PSNODE_KILL_PROCESS      0x0010
#define FORT_PSNODE_KILL_CHILD        0x0020
#define FORT_PSNODE_IS_SVCHOST        0x0040

typedef struct fort_ps_opt
{
    UINT16 volatile flags;

    FORT_APP_PATH_DRIVE ps_drive;
} FORT_PS_OPT, *PFORT_PS_OPT;

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API void fort_pstree_open(PFORT_PSTREE ps_tree);

FORT_API void fort_pstree_close(PFORT_PSTREE ps_tree);

FORT_API void fort_pstree_enum_processes(PFORT_PSTREE ps_tree);

FORT_API BOOL fort_pstree_get_proc_name(
        PFORT_PSTREE ps_tree, DWORD processId, PFORT_APP_PATH path, PFORT_PS_OPT ps_opt);

FORT_API void fort_pstree_update_services(
        PFORT_PSTREE ps_tree, PCFORT_SERVICE_INFO_LIST services, ULONG data_len);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTPS_H
