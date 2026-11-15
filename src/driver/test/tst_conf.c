/* Fort Firewall Driver: Test: Configuration */

#include "test.h"

#include "../common/fortconf.h"

#define TEST_GROUP_BIT(id) (1u << ((id) - 1))

static void test_conf_groups(void)
{
    /* Groups 1, 2, 31, 32 are non-exclusive; 3, 4 are exclusive. Enabled: 1, 3, 31. */
    const FORT_CONF_GROUPS groups = {
        .mask = TEST_GROUP_BIT(1) | TEST_GROUP_BIT(2) | TEST_GROUP_BIT(3) | TEST_GROUP_BIT(4)
                | TEST_GROUP_BIT(31) | TEST_GROUP_BIT(32),
        .enabled_mask = TEST_GROUP_BIT(1) | TEST_GROUP_BIT(3) | TEST_GROUP_BIT(31),
        .exclusive_mask = TEST_GROUP_BIT(3) | TEST_GROUP_BIT(4),
    };

#define TEST_BLOCKED(m) fort_conf_groups_mask_blocked(&groups, (m))

    assert(!TEST_BLOCKED(0)); /* not in any Group */

    assert(!TEST_BLOCKED(TEST_GROUP_BIT(1))); /* non-exclusive enabled */
    assert(TEST_BLOCKED(TEST_GROUP_BIT(2))); /* non-exclusive disabled */
    assert(!TEST_BLOCKED(TEST_GROUP_BIT(1) | TEST_GROUP_BIT(2))); /* any Group enabled */

    assert(!TEST_BLOCKED(TEST_GROUP_BIT(3))); /* exclusive enabled */
    assert(TEST_BLOCKED(TEST_GROUP_BIT(4))); /* exclusive disabled */
    assert(!TEST_BLOCKED(TEST_GROUP_BIT(3) | TEST_GROUP_BIT(4))); /* any exclusive enabled */

    assert(TEST_BLOCKED(TEST_GROUP_BIT(1) | TEST_GROUP_BIT(4))); /* exclusive beats non-exclusive */
    assert(!TEST_BLOCKED(TEST_GROUP_BIT(1) | TEST_GROUP_BIT(3))); /* both satisfied */
    assert(!TEST_BLOCKED(TEST_GROUP_BIT(2) | TEST_GROUP_BIT(3))); /* exclusive enabled */
    assert(TEST_BLOCKED(TEST_GROUP_BIT(2) | TEST_GROUP_BIT(4))); /* no Group enabled */
    assert(!TEST_BLOCKED(TEST_GROUP_BIT(2) | TEST_GROUP_BIT(3) | TEST_GROUP_BIT(4)));
    assert(TEST_BLOCKED(TEST_GROUP_BIT(1) | TEST_GROUP_BIT(31) | TEST_GROUP_BIT(4)));

    assert(!TEST_BLOCKED(TEST_GROUP_BIT(20))); /* removed Group is ignored */
    assert(TEST_BLOCKED(TEST_GROUP_BIT(2) | TEST_GROUP_BIT(20)));

    assert(!TEST_BLOCKED(TEST_GROUP_BIT(2) | TEST_GROUP_BIT(31)));
    assert(TEST_BLOCKED(TEST_GROUP_BIT(32))); /* the last Group id */
    assert(!TEST_BLOCKED(TEST_GROUP_BIT(1) | TEST_GROUP_BIT(32)));

#undef TEST_BLOCKED
}

static void tst_conf_main(void)
{
    test_conf_groups();
}
