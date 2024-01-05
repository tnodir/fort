#ifndef FORTPROV_H
#define FORTPROV_H

#include "common.h"

typedef struct fort_prov_boot_conf
{
    union {
        UINT32 v;

        struct
        {
            UINT32 boot_filter : 1;
            UINT32 filter_locals : 1;
        };
    };
} FORT_PROV_BOOT_CONF, *PFORT_PROV_BOOT_CONF;

#define fort_prov_open(engine)         FwpmEngineOpen0(NULL, RPC_C_AUTHN_WINNT, NULL, NULL, (engine))
#define fort_prov_close(engine)        FwpmEngineClose0(engine)
#define fort_prov_trans_begin(engine)  FwpmTransactionBegin0((engine), 0)
#define fort_prov_trans_commit(engine) FwpmTransactionCommit0(engine)
#define fort_prov_trans_abort(engine)  FwpmTransactionAbort0(engine)

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API void fort_prov_init(void);

FORT_API DWORD fort_prov_trans_open(HANDLE *engine);

FORT_API DWORD fort_prov_trans_close(HANDLE engine, DWORD status);

FORT_API void fort_prov_flow_unregister(HANDLE engine);

FORT_API void fort_prov_unregister(HANDLE engine);

FORT_API void fort_prov_trans_unregister(void);

FORT_API DWORD fort_prov_register(HANDLE engine, const FORT_PROV_BOOT_CONF boot_conf);

FORT_API DWORD fort_prov_trans_register(const FORT_PROV_BOOT_CONF boot_conf);

FORT_API BOOL fort_prov_get_boot_conf(HANDLE engine, PFORT_PROV_BOOT_CONF boot_conf);

FORT_API DWORD fort_prov_flow_register(HANDLE engine, BOOL filter_packets);

FORT_API void fort_prov_reauth(HANDLE engine);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTPROV_H
