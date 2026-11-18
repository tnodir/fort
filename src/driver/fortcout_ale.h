#ifndef FORTCOUT_ALE_H
#define FORTCOUT_ALE_H

#include "fortdrv.h"

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API void NTAPI fort_callout_ale_classify(const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, PVOID layerData,
        const FWPS_FILTER0 *filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0 *classifyOut);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTCOUT_ALE_H
