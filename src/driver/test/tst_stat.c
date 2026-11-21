/* Fort Firewall Driver: Test: Traffic Statistics */

#include "test.h"

#include "../common/fortdef.h"
#include "../common/fortlog.h"
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

/* The timer's flush loop ends only by the exact count of the active list's live flows */
static void test_stat_flow_active_count_check(PFORT_STAT stat)
{
    UINT32 live_count = 0;

    for (PCFORT_FLOW flow = stat->flow_active; flow != NULL; flow = flow->next_active) {
        if ((flow->opt.flags & FORT_FLOW_DELETED) == 0) {
            ++live_count;
        }
    }

    assert(stat->flow_active_count == live_count);
}

/* Flushes one flow's bytes */
static FORT_FLOW_STAT test_stat_flow_flush(PFORT_STAT stat)
{
    char out[FORT_LOG_FLOW_STAT_SIZE];

    fort_stat_flows_traf_flush(stat, /*flow_count=*/1, out);
    test_stat_flow_active_count_check(stat);

    FORT_FLOW_STAT flow_stat;
    fort_log_flow_stat_read(out, &flow_stat);

    return flow_stat;
}

static void test_stat_flow_traf(void)
{
    FORT_STAT stat = { 0 };
    fort_stat_open(&stat);
    fort_stat_log_update(&stat, TRUE);

    FORT_CONF_META_CONN conn = { .process_id = 11, .flow_id = 1 };

    test_stat_flow_associate_conn(&stat, &conn);

    PFORT_FLOW flow = tommy_arrayof_ref(&stat.flows, 0);
    assert((fort_flow_flags(flow) & FORT_FLOW_DELETED) == 0);

    PFORT_STAT_PROC proc = tommy_arrayof_ref(&stat.procs, flow->opt.proc_index);

    const UINT64 flowContext = (UINT64) flow;

    /* The flow's and the process's bytes, the flow is in the active list once */
    assert(fort_flow_classify(&stat, flowContext, 100, /*inbound=*/TRUE));
    assert(fort_flow_classify(&stat, flowContext, 50, /*inbound=*/FALSE));
    assert(flow->traf.in_bytes == 100);
    assert(flow->traf.out_bytes == 50);
    assert(proc->traf.in_bytes == 100);
    assert(stat.flow_active == flow);
    assert(flow->next_active == NULL);
    assert(stat.flow_active_count == 1);

    /* The App doesn't collect the traffic: only the flow's bytes */
    conn.app.data.flags.found = TRUE;
    test_stat_flow_associate_conn(&stat, &conn);

    assert(fort_flow_classify(&stat, flowContext, 10, /*inbound=*/TRUE));
    assert(flow->traf.in_bytes == 110);
    assert(proc->traf.in_bytes == 100);

    /* The reauthorization keeps the flow in the active list */
    assert((fort_flow_flags(flow) & FORT_FLOW_TRAF_PENDING) != 0);

    /* The flow's bytes' overflow: nothing is counted till the repeat after the flush */
    conn.app.data.flags.log_stat = TRUE;
    test_stat_flow_associate_conn(&stat, &conn);

    flow->traf.in_bytes = MAXUINT32 - 5;
    assert(!fort_flow_classify(&stat, flowContext, 10, /*inbound=*/TRUE));
    assert(proc->traf.in_bytes == 100);

    /* The process's bytes' overflow */
    flow->traf.in_bytes = 110;
    proc->traf.in_bytes = MAXUINT32 - 5;
    assert(!fort_flow_classify(&stat, flowContext, 10, /*inbound=*/TRUE));
    assert(flow->traf.in_bytes == 110);

    proc->traf.in_bytes = 0;
    assert(fort_flow_classify(&stat, flowContext, 10, /*inbound=*/TRUE));
    assert(flow->traf.in_bytes == 120);

    /* The flush writes the flow's bytes and clears them */
    FORT_FLOW_STAT flow_stat = test_stat_flow_flush(&stat);
    assert(flow_stat.flow_id == conn.flow_id);
    assert(flow_stat.traf.in_bytes == 120);
    assert(flow_stat.traf.out_bytes == 50);
    assert(flow->traf.v == 0);
    assert((fort_flow_flags(flow) & FORT_FLOW_TRAF_PENDING) == 0);
    assert(stat.flow_active == NULL);
    assert(stat.flow_active_count == 0);

    fort_stat_close(&stat);

    printf("test_stat_flow_traf: OK\n");
}

static void test_stat_flow_traf_deleted(void)
{
    FORT_STAT stat = { 0 };
    fort_stat_open(&stat);
    fort_stat_log_update(&stat, TRUE);

    FORT_CONF_META_CONN conn = { .process_id = 11, .flow_id = 1 };
    test_stat_flow_associate_conn(&stat, &conn);

    PFORT_FLOW flow = tommy_arrayof_ref(&stat.flows, 0);

    const UINT64 flowContext = (UINT64) flow;

    assert(fort_flow_classify(&stat, flowContext, 5, /*inbound=*/TRUE));

    fort_flow_delete(&stat, flowContext);

    /* The deleted flow of the active list isn't counted, its slot is freed by the flush */
    assert((fort_flow_flags(flow) & FORT_FLOW_DELETED) != 0);
    assert(stat.flow_active == flow);
    assert(stat.flow_active_count == 0);
    assert(stat.flow_free == NULL);

    fort_stat_flows_traf_flush(&stat, /*flow_count=*/0, /*out=*/NULL);
    test_stat_flow_active_count_check(&stat);
    assert(stat.flow_active == NULL);
    assert(stat.flow_free == flow);

    /* The reused slot's bytes start anew */
    conn.flow_id = 2;
    test_stat_flow_associate_conn(&stat, &conn);
    assert(flow->flow_id == conn.flow_id);
    assert((fort_flow_flags(flow) & FORT_FLOW_DELETED) == 0);
    assert((fort_flow_flags(flow) & FORT_FLOW_TRAF_PENDING) == 0);
    assert(flow->traf.v == 0);

    /* The flow whose CONN isn't logged isn't counted */
    fort_flow_conn_unlog(&stat, conn.flow_id);
    assert(fort_flow_classify(&stat, flowContext, 10, /*inbound=*/TRUE));
    assert(flow->traf.v == 0);
    assert(stat.flow_active == NULL);
    fort_flow_delete(&stat, flowContext);

    /* The master's disconnection clears the active list */
    conn.flow_id = 3;
    test_stat_flow_associate_conn(&stat, &conn);
    assert(flow->flow_id == conn.flow_id);

    assert(fort_flow_classify(&stat, flowContext, 10, /*inbound=*/TRUE));
    assert(stat.flow_active == flow);

    fort_stat_log_update(&stat, FALSE);
    test_stat_flow_active_count_check(&stat);
    assert(stat.flow_active == NULL);
    assert(stat.flow_active_count == 0);
    assert(flow->traf.v == 0);
    assert((fort_flow_flags(flow) & FORT_FLOW_TRAF_PENDING) == 0);

    fort_stat_close(&stat);

    printf("test_stat_flow_traf_deleted: OK\n");
}

static void test_stat_flows_traf_flush(void)
{
    FORT_STAT stat = { 0 };
    fort_stat_open(&stat);
    fort_stat_log_update(&stat, TRUE);

    FORT_CONF_META_CONN conn = { .process_id = 11, .flow_id = 0 };

    /* The flows in the active list: 3, 2, 1 */
    for (int i = 0; i < 3; ++i) {
        ++conn.flow_id;
        test_stat_flow_associate_conn(&stat, &conn);

        PFORT_FLOW flow = tommy_arrayof_ref(&stat.flows, i);
        assert(fort_flow_classify(&stat, (UINT64) flow, 10 + i, /*inbound=*/FALSE));
    }

    PFORT_FLOW flow1 = tommy_arrayof_ref(&stat.flows, 0);
    PFORT_FLOW flow2 = tommy_arrayof_ref(&stat.flows, 1);

    /* The deleted flow 2 isn't counted */
    fort_flow_delete(&stat, (UINT64) flow2);
    assert(stat.flow_active_count == 2);

    /* A page of one flow: the deleted flow after it is freed, the next live flow stays */
    FORT_FLOW_STAT flow_stat = test_stat_flow_flush(&stat);
    assert(flow_stat.flow_id == 3);
    assert(flow_stat.traf.out_bytes == 12);
    assert(stat.flow_active == flow1);
    assert(stat.flow_active_count == 1);
    assert(stat.flow_free == flow2);

    flow_stat = test_stat_flow_flush(&stat);
    assert(flow_stat.flow_id == 1);
    assert(flow_stat.traf.out_bytes == 10);
    assert(stat.flow_active == NULL);
    assert(stat.flow_active_count == 0);

    fort_stat_close(&stat);

    printf("test_stat_flows_traf_flush: OK\n");
}

static void tst_stat_main(void)
{
    test_stat_proc_named();
    test_stat_flow_conn_log();
    test_stat_flow_reauth_blocked();
    test_stat_flow_traf();
    test_stat_flow_traf_deleted();
    test_stat_flows_traf_flush();
}
