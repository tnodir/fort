#pragma once

#include <googletest.h>

#include <control/command/controlcommandutil.h>

class ControlCommandUtilTest : public Test
{
    // Test interface
protected:
    void SetUp();
    void TearDown();

    QVariantMap fields() const;
};

void ControlCommandUtilTest::SetUp() { }

void ControlCommandUtilTest::TearDown() { }

QVariantMap ControlCommandUtilTest::fields() const
{
    return { { "flag", false }, { "seconds", QVariant::fromValue(quint16(30)) }, { "count", 5 },
        { "id", QVariant::fromValue(quint8(7)) }, { "name", "A" }, { "ids", QVariantList { 2 } } };
}

TEST_F(ControlCommandUtilTest, parseFieldArgs)
{
    QVariantMap map = fields();
    QString errorMessage;

    ASSERT_TRUE(ControlCommandUtil::parseFieldArgs(
            { "set", "flag=1", "seconds=65535", "count=-2", "id=255", "name=B C=D", "ids=1,32" }, 1,
            map, errorMessage));

    ASSERT_EQ(map["flag"], QVariant(true));
    ASSERT_EQ(map["seconds"], QVariant::fromValue(quint16(65535)));
    ASSERT_EQ(map["count"], QVariant(-2));
    ASSERT_EQ(map["id"], QVariant::fromValue(quint8(255)));
    ASSERT_EQ(map["name"], QVariant("B C=D"));
    ASSERT_EQ(map["ids"], QVariant(QVariantList { 1, 32 }));
    ASSERT_EQ(ControlCommandUtil::idsMask(map["ids"]), 0x80000001);

    ASSERT_TRUE(ControlCommandUtil::parseFieldArgs({ "flag=false" }, 0, map, errorMessage));
    ASSERT_EQ(map["flag"], QVariant(false));

    ASSERT_TRUE(ControlCommandUtil::parseFieldArgs({ "name=", "ids=" }, 0, map, errorMessage));
    ASSERT_EQ(map["name"], QVariant(QString()));
    ASSERT_EQ(map["ids"], QVariant(QVariantList {}));
}

TEST_F(ControlCommandUtilTest, parseFieldArgsBad)
{
    const QStringList badArgs = { "flag", "=1", "unknown=1", "flag=2", "flag=yes", "seconds=65536",
        "seconds=-1", "count=1.5", "count=", "id=256", "ids=0", "ids=33", "ids=1,x" };

    for (const QString &arg : badArgs) {
        QVariantMap map = fields();
        QString errorMessage;

        ASSERT_FALSE(ControlCommandUtil::parseFieldArgs({ arg }, 0, map, errorMessage))
                << arg.toStdString();
        ASSERT_TRUE(errorMessage.endsWith(": " + arg)) << errorMessage.toStdString();
        ASSERT_EQ(map, fields()) << arg.toStdString();
    }
}

TEST_F(ControlCommandUtilTest, nameIndex)
{
    const QStringList names = { "A", "B" };
    QString errorMessage;

    ASSERT_EQ(ControlCommandUtil::nameIndex(fields(), "name", names, errorMessage), 0);
    ASSERT_TRUE(errorMessage.isEmpty());

    ASSERT_EQ(ControlCommandUtil::nameIndex({ { "name", "C" } }, "name", names, errorMessage), -1);
    ASSERT_EQ(errorMessage, "Invalid value: name=C");
}

TEST_F(ControlCommandUtilTest, maskIds)
{
    ASSERT_EQ(ControlCommandUtil::maskIds(0), QVariant(QVariantList {}));
    ASSERT_EQ(ControlCommandUtil::maskIds(0x80000005), QVariant(QVariantList { 1, 3, 32 }));
    ASSERT_EQ(ControlCommandUtil::idsMask(QVariantList { 1, 3, 32 }), 0x80000005);
}

TEST_F(ControlCommandUtilTest, fieldsToText)
{
    ASSERT_EQ(ControlCommandUtil::fieldsToText(fields()),
            "count=5 flag=0 id=7 ids=2 name=A seconds=30");
    ASSERT_EQ(ControlCommandUtil::fieldsToText({ { "flag", true } }), "flag=1");
    ASSERT_EQ(ControlCommandUtil::fieldsToText({ { "ids", QVariantList { 1, 3 } } }), "ids=1,3");
    ASSERT_EQ(
            ControlCommandUtil::fieldsToText({ { "a", "B C" }, { "b", "D\"E" }, { "c", "F\nG" } }),
            "a=\"B C\" b=\"D\"\"E\" c=\"F\nG\"");
    ASSERT_EQ(ControlCommandUtil::fieldsToText({}), QString());
}
