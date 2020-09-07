#ifndef FORTTDS_H
#define FORTTDS_H

#include "fortdrv.h"

#define TOMMY_API FORT_API

#define tommy_assert NT_ASSERT

#define tommy_malloc  fort_tommy_malloc
#define tommy_free    fort_tommy_free
#define tommy_calloc  fort_tommy_calloc
#define tommy_realloc fort_tommy_realloc

FORT_API PVOID fort_tommy_malloc(SIZE_T size);

FORT_API void fort_tommy_free(PVOID p);

FORT_API PVOID fort_tommy_calloc(SIZE_T count, SIZE_T element_size);

FORT_API PVOID fort_tommy_realloc(PVOID p, SIZE_T new_size);

#include "../3rdparty/tommyds/tommyarrayof.h"
#include "../3rdparty/tommyds/tommylist.h"
#include "../3rdparty/tommyds/tommyhash.h"
#include "../3rdparty/tommyds/tommyhashdyn.h"

#endif // FORTTDS_H
