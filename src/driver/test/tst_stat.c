/* Fort Firewall Driver: Test: Traffic Statistics */

#include "test.h"

#include "../common/fortdef.h"
#include "../fortstat.h"

/* Associates the connection's flow */
static FORT_FLOW_ASSOC test_stat_flow_associate_conn(PFORT_STAT stat, PCFORT_CONF_META_CONN conn)
{
    FORT_FLOW_ASSOC assoc = { 0 };

    assert(fort_flow_associate(stat, conn, &assoc) == STATUS_SUCCESS);

    return assoc;
}

/* Associates the process's new flow, returns how its PROC_NEW is logged */
static FORT_FLOW_PROC_LOG_TYPE test_stat_flow_associate(
        PFORT_STAT stat, PFORT_CONF_META_CONN conn, PVOID ps_name)
{
    conn->ps_name = ps_name;
    ++conn->flow_id;

    return test_stat_flow_associate_conn(stat, conn).proc_log;
}

/* The connection logs the process's PROC_NEW, the next connection finds it written */
static void test_stat_proc_log_new(PFORT_STAT stat, PFORT_CONF_META_CONN conn, PVOID ps_name)
{
    assert(test_stat_flow_associate(stat, conn, ps_name) == FORT_FLOW_PROC_LOG_NEW);

    fort_flow_proc_written(stat, conn);
    assert(test_stat_flow_associate(stat, conn, ps_name) == FORT_FLOW_PROC_LOG_WRITTEN);
}

static void test_stat_proc_named(void)
{
    FORT_STAT stat = { 0 };
    fort_stat_open(&stat);
    fort_stat_log_update(&stat, TRUE);

    FORT_CONF_META_CONN conn = { .process_id = 11 };
    static CHAR ps_name[1]; /* any process's name */

    /* The process's first flow logs it, the next ones wait for its writing */
    assert(test_stat_flow_associate(&stat, &conn, NULL) == FORT_FLOW_PROC_LOG_NEW);
    assert(test_stat_flow_associate(&stat, &conn, NULL) == FORT_FLOW_PROC_LOG_PENDING);

    /* The process's name appears: logged again after the PROC_NEW's writing */
    assert(test_stat_flow_associate(&stat, &conn, ps_name) == FORT_FLOW_PROC_LOG_PENDING);

    conn.ps_name = NULL;
    fort_flow_proc_written(&stat, &conn);
    test_stat_proc_log_new(&stat, &conn, ps_name);

    /* A new process of the PID with another ps name: the old PROC_NEW's writing doesn't mark it */
    assert(test_stat_flow_associate(&stat, &conn, NULL) == FORT_FLOW_PROC_LOG_NEW);
    fort_flow_proc_unlog(&stat, conn.process_id);
    assert(test_stat_flow_associate(&stat, &conn, ps_name) == FORT_FLOW_PROC_LOG_NEW);

    conn.ps_name = NULL;
    fort_flow_proc_written(&stat, &conn);
    assert(test_stat_flow_associate(&stat, &conn, ps_name) == FORT_FLOW_PROC_LOG_PENDING);

    conn.ps_name = ps_name;
    fort_flow_proc_written(&stat, &conn);
    assert(test_stat_flow_associate(&stat, &conn, ps_name) == FORT_FLOW_PROC_LOG_WRITTEN);

    /* The PROC_NEW's write failed: logged again */
    assert(test_stat_flow_associate(&stat, &conn, NULL) == FORT_FLOW_PROC_LOG_NEW);
    fort_flow_proc_unlog(&stat, conn.process_id);
    test_stat_proc_log_new(&stat, &conn, NULL);

    /* The service restarts: logged again */
    fort_stat_log_update(&stat, FALSE);
    fort_stat_log_update(&stat, TRUE);
    test_stat_proc_log_new(&stat, &conn, NULL);

    fort_stat_close(&stat);

    printf("test_stat_proc_named: OK\n");
}

/* Is the reauthorized flow's CONN logged? */
static BOOL test_stat_flow_conn_logged(PFORT_STAT stat, PCFORT_CONF_META_CONN conn)
{
    return test_stat_flow_associate_conn(stat, conn).conn_logged;
}

static void test_stat_flow_conn_log(void)
{
    FORT_STAT stat = { 0 };
    fort_stat_open(&stat);
    fort_stat_log_update(&stat, TRUE);

    FORT_CONF_META_CONN conn = { .process_id = 11, .flow_id = 1 };

    /* The flow's first connection logs it, its reauthorizations don't */
    assert(!test_stat_flow_conn_logged(&stat, &conn));
    assert(test_stat_flow_conn_logged(&stat, &conn));
    assert(test_stat_flow_conn_logged(&stat, &conn));

    /* Another flow of the process */
    conn.flow_id = 2;
    assert(!test_stat_flow_conn_logged(&stat, &conn));
    assert(test_stat_flow_conn_logged(&stat, &conn));

    /* The CONN's write failed: logged again */
    fort_flow_conn_unlog(&stat, conn.flow_id);
    assert(!test_stat_flow_conn_logged(&stat, &conn));
    assert(test_stat_flow_conn_logged(&stat, &conn));

    /* The other flow stays logged */
    conn.flow_id = 1;
    assert(test_stat_flow_conn_logged(&stat, &conn));

    /* A deleted flow's failed write */
    fort_flow_conn_unlog(&stat, /*flow_id=*/3);

    /* The service restarts: all flows are logged again */
    fort_stat_log_update(&stat, FALSE);
    fort_stat_log_update(&stat, TRUE);
    assert(!test_stat_flow_conn_logged(&stat, &conn));
    assert(test_stat_flow_conn_logged(&stat, &conn));

    conn.flow_id = 2;
    assert(!test_stat_flow_conn_logged(&stat, &conn));
    assert(test_stat_flow_conn_logged(&stat, &conn));

    fort_stat_close(&stat);

    printf("test_stat_flow_conn_log: OK\n");
}

static void test_stat_flow_reauth_blocked(void)
{
    FORT_STAT stat = { 0 };
    fort_stat_open(&stat);
    fort_stat_log_update(&stat, TRUE);

    FORT_CONF_META_CONN conn = {
        .process_id = 11,
        .flow_id = 1,
        .reason = FORT_CONN_REASON_ASK_ALLOWED,
    };

    /* The flow allowed once by the user */
    assert(!test_stat_flow_conn_logged(&stat, &conn));
    assert(fort_flow_ask_allowed(&stat, conn.flow_id));

    /* Its reauthorization is allowed by a Rule: still allowed once */
    conn.reason = FORT_CONN_REASON_RULE;
    assert(test_stat_flow_conn_logged(&stat, &conn));
    assert(fort_flow_ask_allowed(&stat, conn.flow_id));

    /* Its reauthorization is blocked: logged and asked again */
    fort_flow_reauth_blocked(&stat, conn.flow_id);
    assert(!fort_flow_ask_allowed(&stat, conn.flow_id));
    assert(!test_stat_flow_conn_logged(&stat, &conn));
    assert(!fort_flow_ask_allowed(&stat, conn.flow_id));

    /* A deleted flow's blocked reauthorization */
    fort_flow_reauth_blocked(&stat, /*flow_id=*/2);

    fort_stat_close(&stat);

    printf("test_stat_flow_reauth_blocked: OK\n");
}

static void tst_stat_main(void)
{
    test_stat_proc_named();
    test_stat_flow_conn_log();
    test_stat_flow_reauth_blocked();
}
