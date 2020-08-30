#pragma once

#include <QSignalSpy>

#include <googletest.h>

#include <task/taskzonedownloader.h>
#include <util/fileutil.h>
#include <util/net/ip4range.h>
#include <util/net/netutil.h>

class NetUtilTest : public Test
{
    // Test interface
protected:
    void SetUp();
    void TearDown();
};

void NetUtilTest::SetUp() { }

void NetUtilTest::TearDown() { }

TEST_F(NetUtilTest, Ip4Text)
{
    const QString ip4Str("172.16.0.1");

    ASSERT_EQ(NetUtil::ip4ToText(NetUtil::textToIp4(ip4Str)), ip4Str);
}

TEST_F(NetUtilTest, Ip4Ranges)
{
    Ip4Range ip4Range;

    ASSERT_FALSE(ip4Range.fromText("172.16.0.0/33"));
    ASSERT_FALSE(ip4Range.fromText("172.16.0.255/-16"));
    ASSERT_FALSE(ip4Range.fromText("10.0.0.32 - 10.0.0.24"));
    ASSERT_EQ(ip4Range.errorLineNo(), 1);

    ASSERT_TRUE(ip4Range.fromText("172.16.0.1/32"));
    ASSERT_EQ(ip4Range.toText(), QString("172.16.0.1\n"));

    // Simple range
    {
        ASSERT_TRUE(ip4Range.fromText("127.0.0.1\n"
                                      "172.16.0.0/20\n"
                                      "192.168.0.0 - 192.168.255.255\n"));
        ASSERT_EQ(ip4Range.errorLineNo(), 0);
        ASSERT_EQ(ip4Range.pairSize(), 2);
        ASSERT_EQ(ip4Range.ipSize(), 1);

        const Ip4Pair &ipPair1 = ip4Range.pairAt(0);
        ASSERT_EQ(ipPair1.from, NetUtil::textToIp4("172.16.0.0"));
        ASSERT_EQ(ipPair1.to, NetUtil::textToIp4("172.16.15.255"));

        const Ip4Pair &ipPair2 = ip4Range.pairAt(1);
        ASSERT_EQ(ipPair2.from, NetUtil::textToIp4("192.168.0.0"));
        ASSERT_EQ(ipPair2.to, NetUtil::textToIp4("192.168.255.255"));

        ASSERT_EQ(ip4Range.ipAt(0), NetUtil::textToIp4("127.0.0.1"));
    }

    // Merge ranges
    {
        ASSERT_TRUE(ip4Range.fromText("10.0.0.0 - 10.0.0.255\n"
                                      "10.0.0.64 - 10.0.0.128\n"
                                      "10.0.0.128 - 10.0.2.0\n"));
        ASSERT_EQ(ip4Range.ipSize(), 0);
        ASSERT_EQ(ip4Range.pairSize(), 1);

        const Ip4Pair &ipPair1 = ip4Range.pairAt(0);
        ASSERT_EQ(ipPair1.from, NetUtil::textToIp4("10.0.0.0"));
        ASSERT_EQ(ipPair1.to, NetUtil::textToIp4("10.0.2.0"));
    }
}

TEST_F(NetUtilTest, TaskTasix)
{
    const QByteArray buf = FileUtil::readFileData(":/data/tasix-mrlg.html");
    ASSERT_FALSE(buf.isEmpty());

    TaskZoneDownloader tasix;
    tasix.setZoneId(1);
    tasix.setSort(true);
    tasix.setEmptyNetMask(24);
    tasix.setPattern("^\\*\\D{2,5}(\\S+)");

    QString textChecksum;
    const auto text = QString::fromLatin1(buf);
    const auto list = tasix.parseAddresses(text, textChecksum);
    ASSERT_FALSE(textChecksum.isEmpty());
    ASSERT_FALSE(list.isEmpty());

    const QString cachePath("./zones/");

    tasix.setTextChecksum(textChecksum);
    tasix.setCachePath(cachePath);
    ASSERT_TRUE(tasix.storeAddresses(list));

    ASSERT_TRUE(tasix.saveAddressesAsText(cachePath + "tasix-mrlg.txt"));
}
