#ifndef FORTPS_H
#define FORTPS_H

#include "fortdrv.h"

#include "fortcnf.h"
#include "fortpsname.h"
#include "forttds.h"

#define FORT_PSTREE_ACTIVE 0x0001

typedef struct fort_pstree
{
    UCHAR volatile flags;

    UINT32 procs_n;

    PFORT_PSNAMES ps_names; /* the processes' names */

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
#define FORT_PSNODE_CLOSED            0x0080
#define FORT_PSNODE_FOUND             0x8000

typedef UINT16 FORT_PS_FLAGS, *PFORT_PS_FLAGS;

typedef struct fort_ps_opt
{
    FORT_PS_FLAGS flags;

    FORT_APP_PATH_DRIVE path_drive;
} FORT_PS_OPT, *PFORT_PS_OPT;

typedef const FORT_PS_OPT *PCFORT_PS_OPT;

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API void fort_pstree_open(PFORT_PSTREE ps_tree, PFORT_PSNAMES ps_names);

/* Returns FALSE, when the process monitor is still active */
FORT_API BOOL fort_pstree_close(PFORT_PSTREE ps_tree);

FORT_API void fort_pstree_enum_processes(PFORT_PSTREE ps_tree);

/* Returns the process's name, which is referenced until fort_psname_put() */
FORT_API PFORT_PSNAME_NODE fort_pstree_get_proc_name(
        PFORT_PSTREE ps_tree, DWORD processId, PFORT_APP_PATH path, PFORT_PS_OPT ps_opt);

/* The process's Ask to Connect is replaced by the user's allowance */
FORT_API BOOL fort_pstree_proc_allow(PFORT_PSTREE ps_tree, PCFORT_PROC_ALLOW proc_allow);

FORT_API BOOL fort_pstree_proc_allowed(PFORT_PSTREE ps_tree, DWORD processId);

/* The closed master's allowed processes */
FORT_API void fort_pstree_procs_allowed_clear(PFORT_PSTREE ps_tree);

FORT_API void fort_pstree_update_services(
        PFORT_PSTREE ps_tree, PCFORT_SERVICE_INFO_LIST services, ULONG data_len);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTPS_H
