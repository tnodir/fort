/* Fort Firewall Driver: Test: Log Buffer */

#include "test.h"

#include "../fortbuf.h"

#define TEST_BUF_COUNT_MIN 1024 /* blocks */

static UINT32 test_buf_fill(PFORT_BUFFER buf, NTSTATUS *status)
{
    UINT32 count = 0;
    PCHAR out;

    /* Each record takes a whole block */
    while (NT_SUCCESS(*status = fort_buffer_prepare(buf, FORT_BUFFER_SIZE, &out, NULL))) {
        ++count;
    }

    return count;
}

static void test_buf_overflow(void)
{
    FORT_BUFFER buf = { 0 };
    fort_buffer_open(&buf);

    NTSTATUS status;

    const UINT32 count = test_buf_fill(&buf, &status);

    assert(status == STATUS_BUFFER_OVERFLOW);
    assert(count >= TEST_BUF_COUNT_MIN);
    assert(buf.overflow_traced);

    /* The overflow is traced once */
    PCHAR out;
    assert(fort_buffer_prepare(&buf, FORT_BUFFER_SIZE, &out, NULL) == STATUS_BUFFER_OVERFLOW);
    assert(buf.overflow_traced);

    /* The client reads a block: traced again on the next overflow */
    static CHAR read_buf[FORT_BUFFER_SIZE];
    FORT_IRP_INFO irp_info = { .irp = NULL };

    assert(fort_buffer_xmove(&buf, &irp_info, read_buf, sizeof(read_buf)) == STATUS_SUCCESS);
    assert(!buf.overflow_traced);

    assert(test_buf_fill(&buf, &status) == 1);
    assert(status == STATUS_BUFFER_OVERFLOW);
    assert(buf.overflow_traced);

    fort_buffer_close(&buf);

    printf("test_buf_overflow: OK\n");
}

static void tst_buf_main(void)
{
    test_buf_overflow();
}
