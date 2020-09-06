#ifndef WILDMATCH_H
#define WILDMATCH_H

#include "common.h"

#define WM_NOMATCH           1
#define WM_MATCH             0
#define WM_ABORT_ALL         -1
#define WM_ABORT_TO_STARSTAR -2

typedef WCHAR wm_char;

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API int wildmatch(const wm_char *pattern, const wm_char *text);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // WILDMATCH_H
