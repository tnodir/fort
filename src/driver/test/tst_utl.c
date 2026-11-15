/* Fort Firewall Driver: Test: Utilities */

#include "test.h"

#include "../fortutl.h"

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

static BOOL test_utl_command_line_arg_find(
        PCWSTR commandLine, size_t len, PCWSTR argName, PUNICODE_STRING value)
{
    UNICODE_STRING cmd;
    cmd.Length = (USHORT) (len * sizeof(WCHAR));
    cmd.MaximumLength = cmd.Length;
    cmd.Buffer = (PWSTR) commandLine;

    UNICODE_STRING arg;
    RtlInitUnicodeString(&arg, argName);

    return fort_command_line_arg(&cmd, &arg, value);
}

static BOOL test_utl_command_line_arg_equal(PCUNICODE_STRING value, PCWSTR text)
{
    const size_t size = wcslen(text) * sizeof(WCHAR);

    return value->Length == size && RtlCompareMemory(value->Buffer, text, size) == size;
}

static void test_utl_command_line_arg(void)
{
#define TEST_CMD L"svchost.exe -k netsvcs -s Schedule -p"

    UNICODE_STRING value;

    /* The value in the middle */
    assert(test_utl_command_line_arg_find(TEST_CMD, wcslen(TEST_CMD), L"-s ", &value));
    assert(test_utl_command_line_arg_equal(&value, L"Schedule"));

    /* The value at the end */
    assert(test_utl_command_line_arg_find(TEST_CMD, wcslen(TEST_CMD), L"-p", &value));
    assert(test_utl_command_line_arg_equal(&value, L""));

    assert(test_utl_command_line_arg_find(TEST_CMD, wcslen(TEST_CMD), L"-k ", &value));
    assert(test_utl_command_line_arg_equal(&value, L"netsvcs"));

    /* The length cuts the value */
    assert(test_utl_command_line_arg_find(
            TEST_CMD, wcslen(L"svchost.exe -k netsvcs -s Sche"), L"-s ", &value));
    assert(test_utl_command_line_arg_equal(&value, L"Sche"));

    /* The argument is past the length */
    assert(!test_utl_command_line_arg_find(
            TEST_CMD, wcslen(L"svchost.exe -k netsvcs -s"), L"-s ", &value));

    /* No argument */
    assert(!test_utl_command_line_arg_find(TEST_CMD, wcslen(TEST_CMD), L"-x ", &value));

    printf("test_utl_command_line_arg: OK\n");

#undef TEST_CMD
}

static void test_utl_bits(void)
{
    const UINT32 v = fort_bits_duplicate16(0x5555);
    printf("test_utl_bits: v=%x\n", v);
    assert(v == 0x33333333);
}

static void tst_utl_main(void)
{
    test_utl_ascii();
    test_utl_command_line_arg();
    test_utl_bits();
}
