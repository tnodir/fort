#ifndef FORTDRV_H
#define FORTDRV_H

#include "../version/fort_version.h"

#include "common/common.h"

/* WDM for Development in User Mode */
#if !defined(FORT_DRIVER)
#    include "wdm/um_aux_klib.h"
#    include "wdm/um_fwpmk.h"
#    include "wdm/um_fwpsk.h"
#    include "wdm/um_ndis.h"
#    include "wdm/um_ntddk.h"
#    include "wdm/um_ntifs.h"
#    include "wdm/um_wdm.h"
#endif

#if defined(FORT_WIN7_COMPAT)
#    define fort_mem_alloc(size, tag)      ExAllocatePoolWithTag(NonPagedPoolNx, (size), (tag))
#    define fort_mem_alloc_exec(size, tag) ExAllocatePoolWithTag(NonPagedPoolExecute, (size), (tag))
#else
#    define fort_mem_alloc(size, tag)                                                              \
        ExAllocatePool2(POOL_FLAG_UNINITIALIZED | POOL_FLAG_NON_PAGED, (size), (tag))
#    define fort_mem_alloc_exec(size, tag)                                                         \
        ExAllocatePool2(POOL_FLAG_UNINITIALIZED | POOL_FLAG_NON_PAGED_EXECUTE, (size), (tag))
#endif

#define fort_mem_free(p, tag)  ExFreePoolWithTag((p), (tag))
#define fort_mem_free_notag(p) ExFreePool((p))

#define fort_request_complete_info(irp, status, info)                                              \
    do {                                                                                           \
        (irp)->IoStatus.Status = (status);                                                         \
        (irp)->IoStatus.Information = (info);                                                      \
        IoCompleteRequest((irp), IO_NO_INCREMENT);                                                 \
    } while (0)

#define fort_request_complete(irp, status) fort_request_complete_info((irp), (status), 0)

#endif // FORTDRV_H
