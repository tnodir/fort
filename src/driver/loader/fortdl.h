#ifndef FORTDL_H
#define FORTDL_H

#include "../fortdrv.h"

#define FORT_LOADER_POOL_TAG 'LwfF'

#define fortdl_alloc(size) fort_mem_alloc((size), FORT_LOADER_POOL_TAG)
#define fortdl_free(p)     fort_mem_free((p), FORT_LOADER_POOL_TAG)

#endif // FORTDL_H
