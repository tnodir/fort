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
        { "id", QVariant::fromValue(quint8(7)) }, { "name", "A" } };
}

TEST_F(ControlCommandUtilTest, parseFieldArgs)
{
    QVariantMap map = fields();
    QString errorMessage;

    ASSERT_TRUE(ControlCommandUtil::parseFieldArgs(
            { "set", "flag=1", "seconds=65535", "count=-2", "id=255", "name=B C=D" }, 1, map,
            errorMessage));

    ASSERT_EQ(map["flag"], QVariant(true));
    ASSERT_EQ(map["seconds"], QVariant::fromValue(quint16(65535)));
    ASSERT_EQ(map["count"], QVariant(-2));
    ASSERT_EQ(map["id"], QVariant::fromValue(quint8(255)));
    ASSERT_EQ(map["name"], QVariant("B C=D"));

    ASSERT_TRUE(ControlCommandUtil::parseFieldArgs({ "flag=false" }, 0, map, errorMessage));
    ASSERT_EQ(map["flag"], QVariant(false));

    ASSERT_TRUE(ControlCommandUtil::parseFieldArgs({ "name=" }, 0, map, errorMessage));
    ASSERT_EQ(map["name"], QVariant(QString()));
}

TEST_F(ControlCommandUtilTest, parseFieldArgsBad)
{
    const QStringList badArgs = { "flag", "=1", "unknown=1", "flag=2", "flag=yes", "seconds=65536",
        "seconds=-1", "count=1.5", "count=", "id=256" };

    for (const QString &arg : badArgs) {
        QVariantMap map = fields();
        QString errorMessage;

        ASSERT_FALSE(ControlCommandUtil::parseFieldArgs({ arg }, 0, map, errorMessage))
                << arg.toStdString();
        ASSERT_TRUE(errorMessage.endsWith(": " + arg)) << errorMessage.toStdString();
        ASSERT_EQ(map, fields()) << arg.toStdString();
    }
}

TEST_F(ControlCommandUtilTest, fieldsToText)
{
    ASSERT_EQ(ControlCommandUtil::fieldsToText(fields()), "count=5 flag=0 id=7 name=A seconds=30");
    ASSERT_EQ(ControlCommandUtil::fieldsToText({ { "flag", true } }), "flag=1");
    ASSERT_EQ(ControlCommandUtil::fieldsToText({}), QString());
}
