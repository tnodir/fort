/* Fort Firewall Driver: Test */

#ifdef NDEBUG
#    undef NDEBUG
#endif

#include <assert.h>
#include <stdio.h>

#include "../fortcb.h"
#include "../fortutl.h"
#include "../proxycb/fortpcb_drv.h"
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

static NTSTATUS test_major0(PDEVICE_OBJECT device, PIRP irp)
{
    printf("Major: %p %p\n", device, irp);
    return STATUS_SUCCESS;
}

static void test_major(void)
{
    PDRIVER_DISPATCH major_funcs[FORT_DRIVER_MAJOR_FUNC_MAX];

    fort_proxycb_drv_prepare(major_funcs);
    major_funcs[0] = test_major0;
    fort_proxycb_drv_setup(major_funcs);

    PDRIVER_DISPATCH cb = major_funcs[0];

    printf("test_major: src_cb=%p dst_cb=%p\n", cb, test_major0);

    fflush(stdout);

    const int res = cb(NULL, NULL);
    assert(res == STATUS_SUCCESS);
}

static void test_utl_ascii(void)
{
#define TEST_DATA     L"TEsT Йц"
#define TEST_DATA_LEN sizeof(TEST_DATA)

    UNICODE_STRING src;
    src.Length = TEST_DATA_LEN;
    src.MaximumLength = TEST_DATA_LEN;
    src.Buffer = TEST_DATA;

    WCHAR buffer[TEST_DATA_LEN / sizeof(WCHAR)];

    UNICODE_STRING dst;
    dst.Length = sizeof(buffer);
    dst.MaximumLength = sizeof(buffer);
    dst.Buffer = buffer;

    fort_ascii_downcase(&dst, &src);

    assert(RtlCompareMemory(dst.Buffer, L"test Йц", TEST_DATA_LEN) == TEST_DATA_LEN);

#undef TEST_DATA
}

static void test_utl_bits(void)
{
    const UINT32 v = fort_bits_duplicate16(0x5555);
    printf("test_utl_bits: v=%x\n", v);
    assert(v == 0x33333333);
}

int main(int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    test_proxycb();
    test_major();
    test_utl_ascii();
    test_utl_bits();

    return 0;
}
