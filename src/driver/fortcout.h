#ifndef FORTCOUT_H
#define FORTCOUT_H

#include "fortdrv.h"

#include "common/fortconf.h"

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API void fort_callout_classify_block(FWPS_CLASSIFY_OUT0 *classifyOut);

FORT_API void fort_callout_classify_drop(FWPS_CLASSIFY_OUT0 *classifyOut);

FORT_API void fort_callout_classify_permit(
        const FWPS_FILTER0 *filter, FWPS_CLASSIFY_OUT0 *classifyOut);

FORT_API void fort_callout_classify_continue(FWPS_CLASSIFY_OUT0 *classifyOut);

FORT_API NTSTATUS fort_callout_install(PDEVICE_OBJECT device);

FORT_API void fort_callout_remove(void);

FORT_API NTSTATUS fort_callout_force_reauth(const FORT_CONF_FLAGS old_conf_flags);

FORT_API void fort_callout_timer(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTCOUT_H
