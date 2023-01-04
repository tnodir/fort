#ifndef FORTPROV_H
#define FORTPROV_H

#include "common.h"

#define fort_prov_open(engine)         FwpmEngineOpen0(NULL, RPC_C_AUTHN_WINNT, NULL, NULL, (engine))
#define fort_prov_close(engine)        FwpmEngineClose0(engine)
#define fort_prov_trans_begin(engine)  FwpmTransactionBegin0((engine), 0)
#define fort_prov_trans_commit(engine) FwpmTransactionCommit0(engine)
#define fort_prov_trans_abort(engine)  FwpmTransactionAbort0(engine)

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API DWORD fort_prov_trans_close(HANDLE transEngine, DWORD status);

FORT_API void fort_prov_unregister(HANDLE transEngine);

FORT_API void fort_prov_flow_unregister(HANDLE transEngine);

FORT_API BOOL fort_prov_is_boot(void);

FORT_API DWORD fort_prov_register(HANDLE transEngine, BOOL is_boot);

FORT_API DWORD fort_prov_flow_register(HANDLE transEngine, BOOL filter_packets);

FORT_API DWORD fort_prov_reauth(HANDLE transEngine);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTPROV_H
