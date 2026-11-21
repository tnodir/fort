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

typedef struct test_buf_path_lens
{
    UINT16 path_len;
    UINT16 inherit_path_len;
} TEST_BUF_PATH_LENS;

/* Writes the connection's entry, returns its paths' lengths read back */
static TEST_BUF_PATH_LENS test_buf_conn_write_read(PFORT_CONF_META_CONN conn,
        const FORT_BUFFER_CONN_WRITE_TYPE log_type, PFORT_CONF_META_CONN read_conn)
{
    FORT_BUFFER buf = { 0 };
    fort_buffer_open(&buf);

    FORT_IRP_INFO irp_info = { .irp = NULL };
    assert(fort_buffer_conn_write(&buf, conn, &irp_info, log_type) == STATUS_SUCCESS);

    static CHAR read_buf[FORT_BUFFER_SIZE];
    assert(fort_buffer_xmove(&buf, &irp_info, read_buf, sizeof(read_buf)) == STATUS_SUCCESS);

    TEST_BUF_PATH_LENS lens = { 0 };

    if (log_type == FORT_BUFFER_CONN_WRITE_CONN) {
        fort_log_conn_header_read(read_buf, read_conn, &lens.path_len, &lens.inherit_path_len);
    } else {
        FORT_LOG_PROC_NEW_HEADER header;
        fort_log_proc_new_header_read(read_buf, &header);

        lens.path_len = header.path_len;
        lens.inherit_path_len = header.inherit_path_len;

        read_conn->process_id = header.process_id;
        read_conn->parent_process_id = header.parent_process_id;
        read_conn->create_time = header.create_time;
    }

    fort_buffer_close(&buf);

    return lens;
}

static void test_buf_conn_proc_path(void)
{
    const FORT_APP_PATH path = { .len = 4, .buffer = L"ab" };
    const FORT_APP_PATH real_path = { .len = 6, .buffer = L"abc" };

    FORT_CONF_META_CONN conn = {
        .inherited = TRUE,
        .process_id = 5,
        .parent_process_id = 7,
        .create_time = 0x1122334455667788LL,
        .path = path,
        .real_path = real_path,
    };
    FORT_CONF_META_CONN read_conn = { 0 };
    TEST_BUF_PATH_LENS lens;

    /* The CONN of a not logged process has the paths */
    lens = test_buf_conn_write_read(&conn, FORT_BUFFER_CONN_WRITE_CONN, &read_conn);
    assert(lens.path_len == real_path.len);
    assert(lens.inherit_path_len == path.len);
    assert(!read_conn.proc_logged);

    /* The CONN of a logged process goes without the paths */
    conn.proc_logged = TRUE;
    lens = test_buf_conn_write_read(&conn, FORT_BUFFER_CONN_WRITE_CONN, &read_conn);
    assert(lens.path_len == 0);
    assert(lens.inherit_path_len == 0);
    assert(read_conn.proc_logged);

    /* The PROC_NEW has the paths and the process's tree */
    lens = test_buf_conn_write_read(&conn, FORT_BUFFER_CONN_WRITE_PROC_NEW, &read_conn);
    assert(lens.path_len == real_path.len);
    assert(lens.inherit_path_len == path.len);
    assert(read_conn.process_id == conn.process_id);
    assert(read_conn.parent_process_id == conn.parent_process_id);
    assert(read_conn.create_time == conn.create_time);

    printf("test_buf_conn_proc_path: OK\n");
}

static void tst_buf_main(void)
{
    test_buf_overflow();
    test_buf_conn_proc_path();
}
