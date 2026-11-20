/* Fort Firewall Driver: Test: Traffic Statistics */

#include "test.h"

#include "../fortstat.h"

/* Associates the process's new flow, returns whether its PROC_NEW is logged */
static BOOL test_stat_flow_associate(PFORT_STAT stat, PFORT_CONF_META_CONN conn, PVOID ps_name)
{
    BOOL proc_logged = FALSE;

    conn->ps_name = ps_name;
    ++conn->flow_id;

    assert(fort_flow_associate(stat, conn, &proc_logged) == STATUS_SUCCESS);

    return proc_logged;
}

static void test_stat_proc_named(void)
{
    FORT_STAT stat = { 0 };
    fort_stat_open(&stat);
    fort_stat_log_update(&stat, TRUE);

    FORT_CONF_META_CONN conn = { .process_id = 11 };
    static CHAR ps_name[1]; /* any process's name */

    /* The process's first flow logs it */
    assert(!test_stat_flow_associate(&stat, &conn, NULL));
    assert(test_stat_flow_associate(&stat, &conn, NULL));

    /* The process's name appears: logged again */
    assert(!test_stat_flow_associate(&stat, &conn, ps_name));
    assert(test_stat_flow_associate(&stat, &conn, ps_name));

    /* The PROC_NEW's write failed: logged again */
    fort_flow_proc_unlog(&stat, conn.process_id);
    assert(!test_stat_flow_associate(&stat, &conn, ps_name));
    assert(test_stat_flow_associate(&stat, &conn, ps_name));

    /* The service restarts: logged again */
    fort_stat_log_update(&stat, FALSE);
    fort_stat_log_update(&stat, TRUE);
    assert(!test_stat_flow_associate(&stat, &conn, ps_name));
    assert(test_stat_flow_associate(&stat, &conn, ps_name));

    fort_stat_close(&stat);

    printf("test_stat_proc_named: OK\n");
}

static void tst_stat_main(void)
{
    test_stat_proc_named();
}
