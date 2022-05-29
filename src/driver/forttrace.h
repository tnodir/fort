#ifndef FORTTRACE_H
#define FORTTRACE_H

#include "fortdrv.h"

#include "evt/fortevt.h"

#define TRACE(event_code, status, error_value, sequence)                                           \
    fort_trace_event((event_code), (status), (error_value), (sequence))

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API void fort_trace_event(
        NTSTATUS event_code, NTSTATUS status, ULONG error_value, ULONG sequence);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTTRACE_H
