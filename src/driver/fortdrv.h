#ifndef FORTDRV_H
#define FORTDRV_H

#include "common/common.h"

/* WDM for Development in User Mode */
#if !defined(FORT_DRIVER)
#    include "wdm/um_wdm.h"
#    include "wdm/um_ndis.h"
#    include "wdm/um_fwpsk.h"
#    include "wdm/um_fwpmk.h"
#endif

#define fort_mem_alloc(size, tag) ExAllocatePoolWithTag(NonPagedPool, (size), (tag))
#define fort_mem_free(p, tag)     ExFreePoolWithTag((p), (tag))

#endif // FORTDRV_H
