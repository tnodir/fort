#pragma once

#include <QDeadlineTimer>

#include <googletest.h>

#include <util/expiringhash.h>

namespace {

int expiringHashCount(ExpiringHash<int, QString> &hash)
{
    int count = 0;
    hash.forEachValue([&](QString &) { ++count; });
    return count;
}

}

class ExpiringHashTest : public Test
{
    // Test interface
protected:
    void SetUp();
    void TearDown();
};

void ExpiringHashTest::SetUp() { }

void ExpiringHashTest::TearDown() { }

TEST_F(ExpiringHashTest, value)
{
    ExpiringHash<int, QString> hash(60 * 1000);

    hash.insert(1, "a", QDeadlineTimer(QDeadlineTimer::Forever));
    hash.insert(2, "b", QDeadlineTimer()); // expired

    ASSERT_EQ(hash.value(1), "a");
    ASSERT_EQ(hash.value(2), QString());
    ASSERT_EQ(hash.value(2, "x"), "x");
    ASSERT_EQ(hash.value(3, "x"), "x");
}

TEST_F(ExpiringHashTest, take)
{
    ExpiringHash<int, QString> hash(60 * 1000);

    hash.insert(1, "a", QDeadlineTimer(QDeadlineTimer::Forever));
    hash.insert(2, "b", QDeadlineTimer());

    ASSERT_EQ(hash.take(1), "a");
    ASSERT_FALSE(hash.take(1).has_value());
    ASSERT_FALSE(hash.take(2).has_value());
    ASSERT_EQ(expiringHashCount(hash), 0);
}

TEST_F(ExpiringHashTest, purge)
{
    ExpiringHash<int, QString> purgedHash(0);
    ExpiringHash<int, QString> keptHash(60 * 1000);

    for (auto *hash : { &purgedHash, &keptHash }) {
        hash->insert(1, "a", QDeadlineTimer());
        hash->insert(2, "b", QDeadlineTimer(QDeadlineTimer::Forever));
    }

    ASSERT_EQ(expiringHashCount(purgedHash), 1);
    ASSERT_EQ(expiringHashCount(keptHash), 2);
}

TEST_F(ExpiringHashTest, forEachValue)
{
    ExpiringHash<int, QString> hash(60 * 1000);

    hash.insert(1, "a", QDeadlineTimer(QDeadlineTimer::Forever));
    hash.forEachValue([](QString &value) { value = "b"; });

    ASSERT_EQ(hash.value(1), "b");
}
