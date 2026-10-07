#pragma once

#include <QDebug>
#include <QLocale>

#include <googletest.h>

#include <util/formatutil.h>

class FormatUtilTest : public Test
{
    // Test interface
protected:
    void SetUp();
    void TearDown();

private:
    QLocale m_locale;
};

void FormatUtilTest::SetUp()
{
    // The decimal point
    QLocale::setDefault(QLocale::c());
}

void FormatUtilTest::TearDown()
{
    QLocale::setDefault(m_locale);
}

TEST_F(FormatUtilTest, formatSpeed)
{
    ASSERT_EQ(FormatUtil::formatSpeed(0), "0 b/s");
    ASSERT_EQ(FormatUtil::formatSpeed(512), "512 b/s");
    ASSERT_EQ(FormatUtil::formatSpeed(512 * 1024), "512 Kb/s");
    ASSERT_EQ(FormatUtil::formatSpeed(1536 * 1024), "1.5 Mb/s");

    ASSERT_EQ(FormatUtil::formatSpeed(8 * 1024, FormatUtil::SizeTraditionalFormat), "1 KB/s");
    ASSERT_EQ(FormatUtil::formatSpeed(1500 * 1000, FormatUtil::SpeedIecFormat), "1.5 Mib/s");
}

TEST_F(FormatUtilTest, speedPower)
{
    ASSERT_EQ(FormatUtil::getSpeedPower(1023), 0);
    ASSERT_EQ(FormatUtil::getSpeedPower(1024), 1);
    ASSERT_EQ(FormatUtil::getSpeedPower(2 * 1024 * 1024), 2);

    // In bytes
    ASSERT_EQ(FormatUtil::getSpeedPower(8 * 1023, FormatUtil::SizeTraditionalFormat), 0);
    ASSERT_EQ(FormatUtil::getSpeedPower(8 * 1024, FormatUtil::SizeTraditionalFormat), 1);
}

TEST_F(FormatUtilTest, speedValue)
{
    constexpr qint64 mbit = 1024 * 1024;

    // A common unit of several speeds
    ASSERT_EQ(FormatUtil::formatSpeedValue(mbit / 2, 2, 1), "0.5");
    ASSERT_EQ(FormatUtil::formatSpeedValue(mbit, 2, 1), "1");
    ASSERT_EQ(FormatUtil::formatSpeedValue(mbit / 8, 2, 3), "0.125");
    ASSERT_EQ(FormatUtil::formatSpeedValue(mbit / 8, 2, 2), "0.13");

    // In bytes
    ASSERT_EQ(
            FormatUtil::formatSpeedValue(4 * mbit, 2, 1, FormatUtil::SizeTraditionalFormat), "0.5");

    ASSERT_DOUBLE_EQ(FormatUtil::speedInUnit(mbit / 4, 2), 0.25);
    ASSERT_EQ(FormatUtil::formatSpeedUnit(2), "Mb/s");
    ASSERT_EQ(FormatUtil::formatSpeedUnit(1, FormatUtil::SizeTraditionalFormat), "KB/s");
}
