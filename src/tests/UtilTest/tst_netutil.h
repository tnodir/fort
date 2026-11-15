#pragma once

#include <QFileInfo>
#include <QSignalSpy>

#include <googletest.h>

#include <task/taskzonedownloader.h>
#include <util/conf/confbuffer.h>
#include <util/fileutil.h>
#include <util/net/actionrange.h>
#include <util/net/arearange.h>
#include <util/net/dirrange.h>
#include <util/net/hostpattern.h>
#include <util/net/hostrange.h>
#include <util/net/iprange.h>
#include <util/net/ipverrange.h>
#include <util/net/netformatutil.h>
#include <util/net/netutil.h>
#include <util/net/portrange.h>
#include <util/net/protorange.h>

class NetUtilTest : public Test
{
    // Test interface
protected:
    void SetUp();
    void TearDown();
};

void NetUtilTest::SetUp()
{
    NetUtil::windowsSockInit();
}

void NetUtilTest::TearDown()
{
    NetUtil::windowsSockCleanup();
}

TEST_F(NetUtilTest, ip4Text)
{
    const QString ip4Str("172.16.0.1");

    ASSERT_EQ(NetFormatUtil::ip4ToText(NetFormatUtil::textToIp4(ip4Str)), ip4Str);
}

TEST_F(NetUtilTest, ip6Text)
{
    const QString ip6Str("::1");

    ASSERT_EQ(NetFormatUtil::ip6ToText(NetFormatUtil::textToIp6(ip6Str)), ip6Str);
}

TEST_F(NetUtilTest, ipAddressText)
{
    ip_addr_t ip;

    ip.v4 = NetFormatUtil::textToIp4("172.16.0.1");
    ASSERT_EQ(NetFormatUtil::ipToAddressText(ip), "172.16.0.1");

    ip.v6 = NetFormatUtil::textToIp6("::1");
    ASSERT_EQ(NetFormatUtil::ipToAddressText(ip, /*isIPv6=*/true), "[::1]");
}

TEST_F(NetUtilTest, textToIp)
{
    ip_addr_t ip;
    bool isIPv6 = true;

    ASSERT_TRUE(NetFormatUtil::textToIp(" 127.0.0.2 ", ip, isIPv6));
    ASSERT_FALSE(isIPv6);
    ASSERT_EQ(NetFormatUtil::ipToText(ip), "127.0.0.2");
    ASSERT_TRUE(NetUtil::isLoopbackIp(ip, isIPv6));

    ASSERT_TRUE(NetFormatUtil::textToIp("[::1]", ip, isIPv6));
    ASSERT_TRUE(isIPv6);
    ASSERT_EQ(NetFormatUtil::ipToText(ip, isIPv6), "::1");
    ASSERT_TRUE(NetUtil::isLoopbackIp(ip, isIPv6));

    ASSERT_TRUE(NetFormatUtil::textToIp("2001:db8::1", ip, isIPv6));
    ASSERT_FALSE(NetUtil::isLoopbackIp(ip, isIPv6));

    ASSERT_FALSE(NetFormatUtil::textToIp("1.1.1", ip, isIPv6));
}

TEST_F(NetUtilTest, ip6Bytes01)
{
    const ip6_addr_t ip = NetFormatUtil::textToIp6("ff02::1:3");

    ASSERT_EQ(ip.addr32[0], 0x2ff);
    ASSERT_EQ(ip.addr32[1], 0);
    ASSERT_EQ(ip.addr32[2], 0);
    ASSERT_EQ(ip.addr32[3], 0x3000100);
}

TEST_F(NetUtilTest, ip6Bytes02)
{
    const ip6_addr_t ip = NetFormatUtil::textToIp6("fe80::e58c:84f8:a156:2a23");

    ASSERT_EQ(ip.addr32[0], 0x80fe);
    ASSERT_EQ(ip.addr32[1], 0);
    ASSERT_EQ(ip.addr32[2], 0xf8848ce5);
    ASSERT_EQ(ip.addr32[3], 0x232a56a1);
}

TEST_F(NetUtilTest, ip6Mask01)
{
    const ip6_addr_t ip = NetUtil::applyIp6Mask(NetFormatUtil::textToIp6("::2"), 126);

    ASSERT_EQ(ip.addr32[0], 0);
    ASSERT_EQ(ip.addr32[1], 0);
    ASSERT_EQ(ip.addr32[2], 0);
    ASSERT_EQ(ip.addr32[3], 0x03000000);
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
        ASSERT_EQ(ipPair1.from, NetFormatUtil::textToIp4("172.16.0.0"));
        ASSERT_EQ(ipPair1.to, NetFormatUtil::textToIp4("172.16.15.255"));

        const Ip4Pair &ipPair2 = ipRange.pair4At(1);
        ASSERT_EQ(ipPair2.from, NetFormatUtil::textToIp4("192.168.0.0"));
        ASSERT_EQ(ipPair2.to, NetFormatUtil::textToIp4("192.168.255.255"));

        ASSERT_EQ(ipRange.ip4At(0), NetFormatUtil::textToIp4("127.0.0.1"));
    }

    // Merge ranges
    {
        ASSERT_TRUE(ipRange.fromText("10.0.0.0 - 10.0.0.255\n"
                                     "10.0.0.64 - 10.0.0.128\n"
                                     "10.0.0.128 - 10.0.2.0\n"));
        ASSERT_EQ(ipRange.ip4Size(), 0);
        ASSERT_EQ(ipRange.pair4Size(), 1);

        const Ip4Pair &ipPair1 = ipRange.pair4At(0);
        ASSERT_EQ(ipPair1.from, NetFormatUtil::textToIp4("10.0.0.0"));
        ASSERT_EQ(ipPair1.to, NetFormatUtil::textToIp4("10.0.2.0"));
    }

    // Merge ranges up to the max address
    {
        ASSERT_TRUE(ipRange.fromText("0.0.0.0/0\n"
                                     "10.0.0.0/8\n"));
        ASSERT_EQ(ipRange.toText(), QString("0.0.0.0-255.255.255.255\n"));
    }

    // Keep the widest range with the same start
    {
        ASSERT_TRUE(ipRange.fromText("10.0.0.0/8\n"
                                     "10.0.0.0/24\n"
                                     "10.0.0.0\n"));
        ASSERT_EQ(ipRange.toText(), QString("10.0.0.0-10.255.255.255\n"));
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

    ASSERT_TRUE(ipRange.fromText("[::2]/126\n"
                                 "[::1]/126\n"));
    ASSERT_EQ(ipRange.toText(), QString("::1-::3\n"));

    // Merge ranges
    ASSERT_TRUE(ipRange.fromText("2002::/16\n"
                                 "2002:1::/32\n"
                                 "2002:ffff:ffff:ffff:ffff:ffff:ffff:ffff - 2003::1\n"
                                 "::1 - ::5\n"
                                 "::5 - ::7\n"
                                 "::9 - ::a\n"));
    ASSERT_EQ(ipRange.toText(),
            QString("::1-::7\n"
                    "::9-::a\n"
                    "2002::-2003::1\n"));
}

TEST_F(NetUtilTest, portRanges)
{
    PortRange portRange;

    ASSERT_FALSE(portRange.fromText("-16"));
    ASSERT_EQ(portRange.errorLineNo(), 1);

    ASSERT_TRUE(portRange.fromText("1-128"));
    ASSERT_EQ(portRange.toText(), QString("1-128\n"));

    // Keep the widest range with the same start
    ASSERT_TRUE(portRange.fromText("1-128\n"
                                   "1-16\n"));
    ASSERT_EQ(portRange.toText(), QString("1-128\n"));

    // TCP Prots
    portRange.setProtoTcp(true);

    ASSERT_TRUE(portRange.fromText("smtp\n"
                                   "http\n"
                                   "https\n"));
    ASSERT_EQ(portRange.toText(),
            QString("25\n"
                    "80\n"
                    "443\n"));

    ASSERT_TRUE(portRange.fromText("ISO_TSAP-SSL"));
    ASSERT_EQ(portRange.toText(), QString("102-465\n"));
}

TEST_F(NetUtilTest, protocolRanges)
{
    ProtoRange protoRange;

    ASSERT_FALSE(protoRange.fromText("-16"));
    ASSERT_EQ(protoRange.errorLineNo(), 1);

    ASSERT_TRUE(protoRange.fromText("1-128"));
    ASSERT_EQ(protoRange.toText(), QString("1-128\n"));

    ASSERT_TRUE(protoRange.fromText("tcp\n"
                                    "udp\n"));
    ASSERT_EQ(protoRange.toText(),
            QString("TCP\n"
                    "UDP\n"));

    ASSERT_TRUE(protoRange.fromText("HOPOPT-IPV6_FRAG"));
    ASSERT_EQ(protoRange.toText(), QString("0-44\n"));

    ASSERT_TRUE(protoRange.fromText("TP++-A/N"));
    ASSERT_EQ(protoRange.toText(), QString("39-107\n"));

    ASSERT_TRUE(protoRange.fromText("AX.25-RAWSOCKET"));
    ASSERT_EQ(protoRange.toText(), QString("93-255\n"));
}

TEST_F(NetUtilTest, protocolNames)
{
    ProtoRange protoRange;

    ASSERT_TRUE(protoRange.fromText("ICMP\n"
                                    "IGMP\n"
                                    "TCP\n"
                                    "UDP\n"
                                    "ICMPv6\n"
                                    "RAWSOCKET\n"));

    bool ok = false;

    ASSERT_EQ(NetUtil::protocolNumber(u"udp", ok), 17);
    ASSERT_TRUE(ok);

    ASSERT_EQ(NetUtil::protocolNumber(u"255", ok), 255);
    ASSERT_TRUE(ok);

    NetUtil::protocolNumber(u"256", ok);
    ASSERT_FALSE(ok);

    ASSERT_FALSE(protoRange.fromText("300"));
}

TEST_F(NetUtilTest, ipVerRanges)
{
    IpVerRange ipVerRange;

    ASSERT_FALSE(ipVerRange.fromText("1"));
    ASSERT_EQ(ipVerRange.errorLineNo(), 1);

    ASSERT_TRUE(ipVerRange.fromText("4"));
    ASSERT_EQ(ipVerRange.toText(), QString("4\n"));

    ASSERT_TRUE(ipVerRange.fromText("6\n"
                                    "4\n"));
    ASSERT_EQ(ipVerRange.toText(),
            QString("4\n"
                    "6\n"));
}

TEST_F(NetUtilTest, directionRanges)
{
    DirRange dirRange;

    ASSERT_FALSE(dirRange.fromText("1"));
    ASSERT_EQ(dirRange.errorLineNo(), 1);

    ASSERT_TRUE(dirRange.fromText("in"));
    ASSERT_EQ(dirRange.toText(), QString("IN\n"));

    ASSERT_TRUE(dirRange.fromText("out\n"
                                  "in\n"));
    ASSERT_EQ(dirRange.toText(),
            QString("IN\n"
                    "OUT\n"));
}

TEST_F(NetUtilTest, areaRanges)
{
    AreaRange areaRange;

    ASSERT_FALSE(areaRange.fromText("1"));
    ASSERT_EQ(areaRange.errorLineNo(), 1);

    ASSERT_TRUE(areaRange.fromText("lan"));
    ASSERT_EQ(areaRange.toText(), QString("LAN\n"));

    ASSERT_TRUE(areaRange.fromText("inet\n"
                                   "localhost\n"
                                   "lan\n"));
    ASSERT_EQ(areaRange.toText(),
            QString("LOCALHOST\n"
                    "LAN\n"
                    "INET\n"));
}

TEST_F(NetUtilTest, actionRanges)
{
    ActionRange actionRange;

    ASSERT_FALSE(actionRange.fromText("1"));
    ASSERT_EQ(actionRange.errorLineNo(), 1);

    ASSERT_TRUE(actionRange.fromText("drop"));
    ASSERT_EQ(actionRange.actionTypeId(), FORT_RULE_FILTER_ACTION_DROP);
    ASSERT_EQ(actionRange.toText(), QString("DROP\n"));

    // The actions by their indexes
    int index = 0;
    while (const quint8 actionTypeId = ActionRange::actionTypeIdAt(index)) {
        const QString name = ActionRange::actionTypeName(actionTypeId);
        ASSERT_EQ(ActionRange::actionTypeIdByName(name), actionTypeId) << qPrintable(name);
        ASSERT_EQ(ActionRange::actionTypeIndex(actionTypeId), index) << qPrintable(name);
        ++index;
    }
    ASSERT_EQ(index, 4);
    ASSERT_EQ(ActionRange::actionTypeTexts().size(), index);
    ASSERT_EQ(ActionRange::actionTypeIconPaths().size(), index);

    // Not an action's id
    ASSERT_EQ(ActionRange::actionTypeIdAt(-1), 0);
    ASSERT_EQ(ActionRange::actionTypeIndex(0), -1);
    ASSERT_EQ(ActionRange::actionTypeIndex(1 << 4), -1);
    ASSERT_TRUE(ActionRange::actionTypeName(0).isEmpty());
    ASSERT_TRUE(ActionRange::actionTypeName(1 << 4).isEmpty());
}

TEST_F(NetUtilTest, hostRanges)
{
    HostRange hostRange;

    // Bad names
    for (const auto &text :
            { "*", "*.", ".", "a..b", "-a.com", "a-.com", "a b.com", "a.com..", "..", "..a.com",
                    ".*.com", "*..com", "*.*", "a**.com", "-*.com", "ä*.com", ".ads*.com" }) {
        ASSERT_FALSE(hostRange.fromText(text)) << text;
        ASSERT_EQ(hostRange.errorLineNo(), 1) << text;
    }

    const QString label63(63, 'a');
    ASSERT_TRUE(hostRange.fromText(label63 + ".com"));
    ASSERT_FALSE(hostRange.fromText(label63 + "a.com"));

    const QString name253 = QStringList(4, label63).join('.').left(253);
    ASSERT_TRUE(hostRange.fromText(name253));
    ASSERT_FALSE(hostRange.fromText(name253 + "a"));

    ASSERT_FALSE(hostRange.fromText("example.com\n# comment\nbad..name"));
    ASSERT_EQ(hostRange.errorLineNo(), 3);

    // Normalized names
    ASSERT_TRUE(hostRange.fromText(
            "Example.COM.\n*.Sub.Example.com\n_dmarc.example.org\nlocalhost\nexample.com"));
    ASSERT_EQ(hostRange.size(), 4);
    ASSERT_EQ(hostRange.toText(),
            QString("*.sub.example.com\n_dmarc.example.org\nexample.com\nlocalhost\n"));

    // The name and its subdomains
    ASSERT_TRUE(hostRange.fromText(".Example.com.\n*.example.com\n.org"));
    ASSERT_EQ(hostRange.size(), 4);
    ASSERT_EQ(hostRange.toText(), QString("*.example.com\n*.org\nexample.com\norg\n"));

    // The masks
    ASSERT_TRUE(hostRange.fromText("Ads*.Example.com.\n*.example.com\ngoogle.*\n*.*.com\n"
                                   "*telemetry*\nads*.example.com"));
    ASSERT_EQ(hostRange.size(), 5);
    ASSERT_EQ(hostRange.patternNames().size(), 4);
    ASSERT_EQ(hostRange.toText(),
            QString("*.*.com\n*.example.com\n*telemetry*\nads*.example.com\ngoogle.*\n"));

    QString masksText;
    for (int i = 0; i < HostRange::patternNamesMax; ++i) {
        masksText += QString("a%1*.com\n").arg(i);
    }
    ASSERT_TRUE(hostRange.fromText(masksText));
    ASSERT_FALSE(hostRange.fromText(masksText + "example.com\nb*.com"));
    ASSERT_EQ(hostRange.errorLineNo(), HostRange::patternNamesMax + 2);

    // Internationalized names
    ASSERT_TRUE(hostRange.fromText(QString::fromUtf16(u"München.de\n"
                                                      u".Пример.РФ.")));
    ASSERT_EQ(hostRange.toText(),
            QString("*.xn--e1afmkfd.xn--p1ai\nxn--e1afmkfd.xn--p1ai\nxn--mnchen-3ya.de\n"));

    ASSERT_FALSE(hostRange.fromText(QString::fromUtf16(u"ä..com")));
    ASSERT_FALSE(hostRange.fromText(QString(60, QChar(0x00e4)) + ".com"));
}

TEST_F(NetUtilTest, hostPattern)
{
    struct PatternNames
    {
        const char *pattern;
        QVector<const char *> matched;
        QVector<const char *> notMatched;
    };

    const PatternNames list[] = {
        // Inside of a label
        { "ads*.example.com", { "ads.example.com", "ads1.example.com" },
                { "xads1.example.com", "ads1.x.example.com", "a.ads1.example.com",
                        "ads1.example.com.net", "example.com" } },
        { "a.*.com", { "a.b.com" }, { "a.b.c.com", "a.com", "x.a.b.com" } },
        { "a*b*c.com", { "abc.com", "axxbxxc.com", "ababc.com", "abcbc.com" },
                { "axxbxx.com", "abcb.com", "ab.c.com" } },
        // The trailing one
        { "google.*", { "google.com", "google.co.uk" },
                { "google", "www.google.com", "googlex.com" } },
        { "ads*", { "ads", "ads1.example.com" }, { "xads", "a.ads" } },
        // The leading one
        { "*xample.com", { "xample.com", "example.com", "a.b.example.com" },
                { "example.org", "example.com.net", "xample.co" } },
        { "*.ads*.example.com", { "x.ads1.example.com", "y.x.ads.example.com" },
                { "ads1.example.com", "x.ads1.y.example.com" } },
        // Both
        { "*telemetry*", { "telemetry", "telemetry.example.com", "a.xtelemetryx.example.com" },
                { "tele.metry.com", "telemetr.com" } },
        { "*a.b*", { "a.b", "xa.bx.a.by", "c.xa.b.c" }, { "a.xb", "ax.b", "a" } },
    };

    for (const auto &item : list) {
        for (const char *name : item.matched) {
            ASSERT_TRUE(HostPattern::match(item.pattern, name)) << item.pattern << ' ' << name;
        }
        for (const char *name : item.notMatched) {
            ASSERT_FALSE(HostPattern::match(item.pattern, name)) << item.pattern << ' ' << name;
        }
    }
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

TEST_F(NetUtilTest, taskHosts)
{
    TaskZoneDownloader hosts;
    hosts.setZoneId(2);
    hosts.setIsHosts(true);
    hosts.setPattern(R"(^\s*(\S+))");

    QString textChecksum;
    const QString text("# comment\nWWW.Example.com # inline comment\n\n*.example.org\n");
    const auto list = hosts.parseAddresses(text, textChecksum);
    ASSERT_EQ(list.size(), 2);

    const QString cachePath("./zones/");

    hosts.setCachePath(cachePath);
    ASSERT_TRUE(hosts.storeHosts(list));

    // The driver gets an empty address list
    const QByteArray zoneData = hosts.zoneData();
    ASSERT_FALSE(zoneData.isEmpty());

    ASSERT_TRUE(hosts.loadAddresses());
    ASSERT_EQ(hosts.zoneData(), zoneData);

    IpRange ipRange;
    ASSERT_TRUE(ConfBuffer(zoneData).loadZone(ipRange));
    ASSERT_TRUE(ipRange.isEmpty());

    // The cache keeps the names
    const QString filePath(cachePath + "hosts.txt");
    ASSERT_TRUE(hosts.saveAddressesAsText(filePath));

    QString savedText;
    ASSERT_TRUE(FileUtil::readFile(filePath, savedText));
    ASSERT_EQ(savedText, QString("*.example.org\nwww.example.com\n"));

    ASSERT_FALSE(hosts.storeHosts({ u"bad..name" }));
}
