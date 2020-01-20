#include "test.h"

#include "commontest.h"
#include "task/tasktasix.h"
#include "util/fileutil.h"
#include "util/net/ip4range.h"
#include "util/net/netutil.h"

void Test::ip4Text()
{
    const QString ip4Str("172.16.0.1");

    QCOMPARE(NetUtil::ip4ToText(
                 NetUtil::textToIp4(ip4Str)), ip4Str);
}

void Test::ip4Ranges()
{
    Ip4Range ip4Range;

    QVERIFY(!ip4Range.fromText("172.16.0.0/33"));
    QVERIFY(!ip4Range.fromText("172.16.0.255/-16"));
    QVERIFY(!ip4Range.fromText("10.0.0.32 - 10.0.0.24"));
    QCOMPARE(ip4Range.errorLineNo(), 1);

    QVERIFY(ip4Range.fromText("172.16.0.1/32"));
    QCOMPARE(ip4Range.toText(), QString("172.16.0.1\n"));

    // Simple range
    {
        QVERIFY(ip4Range.fromText(
                    "127.0.0.1\n"
                    "172.16.0.0/20\n"
                    "192.168.0.0 - 192.168.255.255\n"
                    ));
        QCOMPARE(ip4Range.errorLineNo(), 0);
        QCOMPARE(ip4Range.pairSize(), 2);
        QCOMPARE(ip4Range.ipSize(), 1);

        const Ip4Pair &ipPair1 = ip4Range.pairAt(0);
        QCOMPARE(ipPair1.from, NetUtil::textToIp4("172.16.0.0"));
        QCOMPARE(ipPair1.to, NetUtil::textToIp4("172.16.15.255"));

        const Ip4Pair &ipPair2 = ip4Range.pairAt(1);
        QCOMPARE(ipPair2.from, NetUtil::textToIp4("192.168.0.0"));
        QCOMPARE(ipPair2.to, NetUtil::textToIp4("192.168.255.255"));

        QCOMPARE(ip4Range.ipAt(0), NetUtil::textToIp4("127.0.0.1"));
    }

    // Merge ranges
    {
        QVERIFY(ip4Range.fromText(
                    "10.0.0.0 - 10.0.0.255\n"
                    "10.0.0.64 - 10.0.0.128\n"
                    "10.0.0.128 - 10.0.2.0\n"
                    ));
        QCOMPARE(ip4Range.ipSize(), 0);
        QCOMPARE(ip4Range.pairSize(), 1);

        const Ip4Pair &ipPair1 = ip4Range.pairAt(0);
        QCOMPARE(ipPair1.from, NetUtil::textToIp4("10.0.0.0"));
        QCOMPARE(ipPair1.to, NetUtil::textToIp4("10.0.2.0"));
    }
}

void Test::taskTasix()
{
    const QByteArray buf = FileUtil::readFileData(STR(PWD) "/data/tasix-mrlg.html");
    QVERIFY(!buf.isEmpty());

    const QStringList list = TaskTasix::parseTasixBuffer(buf);
    QVERIFY(!list.isEmpty());

    //QVERIFY(FileUtil::writeFile(QString(STR(PWD) "/data/tasix-mrlg.out"), list.join('\n')));
}
