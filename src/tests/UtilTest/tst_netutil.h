#pragma once

#include <QFileInfo>
#include <QSignalSpy>

#include <googletest.h>

#include <task/taskzonedownloader.h>
#include <util/fileutil.h>
#include <util/net/iprange.h>
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

TEST_F(NetUtilTest, ip4Text)
{
    const QString ip4Str("172.16.0.1");

    ASSERT_EQ(NetUtil::ip4ToText(NetUtil::textToIp4(ip4Str)), ip4Str);
}

TEST_F(NetUtilTest, ip6Text)
{
    const QString ip6Str("::1");

    ASSERT_EQ(NetUtil::ip6ToText(NetUtil::textToIp6(ip6Str)), ip6Str);
}

TEST_F(NetUtilTest, ip4Ranges)
{
    IpRange ipRange;

    ASSERT_FALSE(ipRange.fromText("172.16.0.0/33"));
    ASSERT_FALSE(ipRange.fromText("172.16.0.255/-16"));
    ASSERT_FALSE(ipRange.fromText("10.0.0.32 - 10.0.0.24"));
    ASSERT_EQ(ipRange.errorLineNo(), 1);

    ASSERT_TRUE(ipRange.fromText("172.16.0.1/32"));
    ASSERT_EQ(ipRange.toText(), QString("172.16.0.1\n"));

    ASSERT_TRUE(ipRange.fromText("172.16.0.1/0"));
    ASSERT_EQ(ipRange.toText(), QString("172.16.0.1-255.255.255.255\n"));

    // Simple range
    {
        ASSERT_TRUE(ipRange.fromText("127.0.0.1\n"
                                     "172.16.0.0/20\n"
                                     "192.168.0.0 - 192.168.255.255\n"));
        ASSERT_EQ(ipRange.errorLineNo(), 0);
        ASSERT_EQ(ipRange.pair4Size(), 2);
        ASSERT_EQ(ipRange.ip4Size(), 1);

        const Ip4Pair &ipPair1 = ipRange.pair4At(0);
        ASSERT_EQ(ipPair1.from, NetUtil::textToIp4("172.16.0.0"));
        ASSERT_EQ(ipPair1.to, NetUtil::textToIp4("172.16.15.255"));

        const Ip4Pair &ipPair2 = ipRange.pair4At(1);
        ASSERT_EQ(ipPair2.from, NetUtil::textToIp4("192.168.0.0"));
        ASSERT_EQ(ipPair2.to, NetUtil::textToIp4("192.168.255.255"));

        ASSERT_EQ(ipRange.ip4At(0), NetUtil::textToIp4("127.0.0.1"));
    }

    // Merge ranges
    {
        ASSERT_TRUE(ipRange.fromText("10.0.0.0 - 10.0.0.255\n"
                                     "10.0.0.64 - 10.0.0.128\n"
                                     "10.0.0.128 - 10.0.2.0\n"));
        ASSERT_EQ(ipRange.ip4Size(), 0);
        ASSERT_EQ(ipRange.pair4Size(), 1);

        const Ip4Pair &ipPair1 = ipRange.pair4At(0);
        ASSERT_EQ(ipPair1.from, NetUtil::textToIp4("10.0.0.0"));
        ASSERT_EQ(ipPair1.to, NetUtil::textToIp4("10.0.2.0"));
    }
}

TEST_F(NetUtilTest, ip6Ranges)
{
    IpRange ipRange;

    ASSERT_FALSE(ipRange.fromText("::1/129"));
    ASSERT_FALSE(ipRange.fromText("::1/-16"));
    ASSERT_EQ(ipRange.errorLineNo(), 1);

    ASSERT_TRUE(ipRange.fromText("::1/128"));
    ASSERT_EQ(ipRange.toText(), QString("::1\n"));

    ASSERT_TRUE(ipRange.fromText("2002::/16"));
    ASSERT_EQ(ipRange.toText(), QString("2002::-2002:ffff:ffff:ffff:ffff:ffff:ffff:ffff\n"));

    ASSERT_TRUE(ipRange.fromText("::2/126\n"
                                 "::1/126\n"));
    ASSERT_EQ(ipRange.toText(),
            QString("::1-::3\n"
                    "::2-::3\n"));
}

TEST_F(NetUtilTest, taskTasix)
{
    const QByteArray buf = FileUtil::readFileData(":/data/tasix-mrlg.html");
    ASSERT_FALSE(buf.isEmpty());

    TaskZoneDownloader tasix;
    tasix.setZoneId(1);
    tasix.setSort(true);
    tasix.setEmptyNetMask(24);
    tasix.setPattern("^\\*\\D{2,5}([\\d./-]{7,})");

    QString textChecksum;
    const auto text = QString::fromLatin1(buf);
    const auto list = tasix.parseAddresses(text, textChecksum);
    ASSERT_FALSE(textChecksum.isEmpty());
    ASSERT_FALSE(list.isEmpty());

    const QString cachePath("./zones/");

    tasix.setTextChecksum(textChecksum);
    tasix.setCachePath(cachePath);
    ASSERT_TRUE(tasix.storeAddresses(list));

    const QFileInfo out(cachePath + "tasix-mrlg.txt");
    ASSERT_TRUE(tasix.saveAddressesAsText(out.filePath()));
    ASSERT_GT(out.size(), 0);
}
