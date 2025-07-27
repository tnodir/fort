#pragma once

#include <QDebug>

#include <googletest.h>

#include <driver/drivercommon.h>
#include <util/conf/confutil.h>

class WildMatchTest : public Test
{
    // Test interface
protected:
    void SetUp();
    void TearDown();
};

void WildMatchTest::SetUp() { }

void WildMatchTest::TearDown() { }

TEST_F(WildMatchTest, hasWildcard)
{
    ASSERT_TRUE(ConfUtil::hasWildcard("*.exe"));
    ASSERT_FALSE(ConfUtil::hasWildcard("C:/a.exe"));
}

TEST_F(WildMatchTest, wildTexts)
{
    ASSERT_TRUE(DriverCommon::wildMatch("*.exe", "/path/a.exe"));
}

TEST_F(WildMatchTest, wildPaths)
{
    ASSERT_TRUE(DriverCommon::wildMatchPath("[A-F]:/a.exe", "C:/a.exe"));
    ASSERT_TRUE(DriverCommon::wildMatchPath("[^D]:/a.exe", "C:/a.exe"));
    ASSERT_FALSE(DriverCommon::wildMatchPath("[^C]:/a.exe", "C:/a.exe"));
}
