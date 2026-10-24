#pragma once

#include <QDebug>

#include <googletest.h>

#include <util/bitutil.h>

class BitUtilTest : public Test
{
    // Test interface
protected:
    void SetUp();
    void TearDown();
};

void BitUtilTest::SetUp() { }

void BitUtilTest::TearDown() { }

TEST_F(BitUtilTest, bitCount)
{
    ASSERT_EQ(BitUtil::bitCount(0x03), 2);
    ASSERT_EQ(BitUtil::bitCount(0x8F), 5);
}

TEST_F(BitUtilTest, firstZeroBit)
{
    ASSERT_EQ(BitUtil::firstZeroBit(0x03), 2);
    ASSERT_EQ(BitUtil::firstZeroBit(0x8F), 4);
}

TEST_F(BitUtilTest, bitScanReverse)
{
    ASSERT_EQ(BitUtil::bitScanReverse(0x01), 0);
    ASSERT_EQ(BitUtil::bitScanReverse(0x8F), 7);
    ASSERT_EQ(BitUtil::bitScanReverse(0x80000001), 31);
}
