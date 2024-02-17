#ifndef FORTTHR_H
#define FORTTHR_H

#include "fortdrv.h"

typedef struct fort_thread
{
    PVOID thread_obj;
} FORT_THREAD, *PFORT_THREAD;

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API NTSTATUS fort_thread_run(
        PFORT_THREAD thread, PKSTART_ROUTINE routine, PVOID context, int priorityIncrement);

FORT_API void fort_thread_wait(PFORT_THREAD thread);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTTHR_H
