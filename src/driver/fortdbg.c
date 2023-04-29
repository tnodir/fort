/* Fort Firewall Debug */

#include "fortdbg.h"

#define FORT_KERNEL_STACK_MIN_SIZE 1024

#define FORT_STATUS_INVALID_STACK 0x000000DC /* DRIVER_INVALID_STACK_ACCESS */

FORT_API void fort_check_stack(const char *func_name, FORT_FUNC_ID func_id)
{
    const UINT32 free_size = (UINT32) IoGetRemainingStackSize();

    if (free_size < FORT_KERNEL_STACK_MIN_SIZE) {
        LOG("Stack Overflow: %s: id=%d remaining=%d\n", func_name, func_id, free_size);

        KeBugCheckEx(FORT_STATUS_INVALID_STACK, 0, 0, 0, 0);
    }
}
