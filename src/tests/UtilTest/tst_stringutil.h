#pragma once

#include <QDebug>

#include <googletest.h>

#include <util/stringutil.h>

class StringUtilTest : public Test
{
    // Test interface
protected:
    void SetUp();
    void TearDown();
};

void StringUtilTest::SetUp() { }

void StringUtilTest::TearDown() { }

TEST_F(StringUtilTest, multiString)
{
    const QStringList list = { "1", "22", "333" };

    QByteArray buffer;
    ASSERT_TRUE(StringUtil::buildMultiString(buffer, list));
    ASSERT_EQ(StringUtil::parseMultiString(buffer), list);
}

TEST_F(StringUtilTest, multiStringBad)
{
    QByteArray buffer;
    ASSERT_FALSE(StringUtil::buildMultiString(buffer, { "1", "", "2" }));
}
