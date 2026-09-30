#ifndef FORTWRK_H
#define FORTWRK_H

#include "fortdrv.h"

#include "fortthr.h"

enum FORT_WORKER_TYPE {
    FORT_WORKER_REAUTH = 0,
    FORT_WORKER_FUNC_COUNT,
};

typedef void (*FORT_WORKER_FUNC)(void);

#define FORT_WORKER_CLOSED 0x01

typedef struct fort_worker
{
    UCHAR volatile flags;
    UCHAR volatile id_bits;

    KEVENT thread_event;
    FORT_THREAD thread;

    FORT_WORKER_FUNC funcs[FORT_WORKER_FUNC_COUNT];
} FORT_WORKER, *PFORT_WORKER;

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API void fort_worker_func_set(
        PFORT_WORKER worker, UCHAR work_id, FORT_WORKER_FUNC worker_func);

FORT_API void fort_worker_queue(PFORT_WORKER worker, UCHAR work_id);

FORT_API NTSTATUS fort_worker_register(PFORT_WORKER worker);

FORT_API void fort_worker_unregister(PFORT_WORKER worker);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTWRK_H
