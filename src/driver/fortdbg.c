/* Fort Firewall Debugging */

#include "fortdbg.h"

#define FORT_KERNEL_STACK_MIN_SIZE 1024

#define FORT_STATUS_INVALID_STACK 0x0000007F /* UNEXPECTED_KERNEL_MODE_TRAP */
#define FORT_TRAP_CODE            0x0000000C /* STACK_FAULT */

FORT_API void fort_check_stack(const char *func_name, FORT_FUNC_ID func_id)
{
    const UINT32 free_size = (UINT32) IoGetRemainingStackSize();

    if (free_size > FORT_KERNEL_STACK_MIN_SIZE)
        return;

    LOG("Stack Overflow: %s: id=%d remaining=%d\n", func_name, func_id, free_size);

    KeBugCheckEx(FORT_STATUS_INVALID_STACK, FORT_TRAP_CODE, (FORT_DEBUG_VERSION << 16) | func_id,
            free_size, 0);
}
