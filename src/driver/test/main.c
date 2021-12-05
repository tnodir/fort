/* Fort Firewall Driver: Test */

#include <assert.h>
#include <stdio.h>

#include "../fortcb.h"
#include "../proxycb/fortpcb_src.h"

#define TEST_CALLBACK_ID 33

typedef int (*TestCallbackFunc)(PVOID p, int i);

static int test_callback(PVOID p, int i)
{
    printf("test_callback: p=%p i=%d\n", p, i);
    return i;
}

static void test_proxycb(void)
{
    FORT_PROXYCB_INFO cbInfo;

    fort_proxycb_src_prepare(&cbInfo);
    fort_callback_setup(&cbInfo);
    fort_proxycb_src_setup(&cbInfo);

    TestCallbackFunc cb = FORT_CALLBACK(TEST_CALLBACK_ID, TestCallbackFunc, test_callback);

    printf("test_proxycb: src_cb=%p dst_cb=%p test_callback=%p\n", cb,
            g_proxyDstCallbacksArray[TEST_CALLBACK_ID], test_callback);

    fflush(stdout);

    const int res = cb(test_callback, TEST_CALLBACK_ID);
    assert(res == TEST_CALLBACK_ID);
}

int main(int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    test_proxycb();

    return 0;
}
