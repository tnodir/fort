#ifndef FORTDRV_H
#define FORTDRV_H

#include "common/common.h"

#define fort_mem_alloc(size, tag) ExAllocatePoolWithTag(NonPagedPool, (size), (tag))
#define fort_mem_free(p, tag)     ExFreePoolWithTag((p), (tag))

#endif // FORTDRV_H
