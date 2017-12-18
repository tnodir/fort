#include "appgroup.h"

AppGroup::AppGroup(QObject *parent) :
    QObject(parent),
    m_enabled(true),
    m_speedLimitIn(0),
    m_speedLimitOut(0)
{
}

void AppGroup::setEnabled(bool enabled)
{
    if (bool(m_enabled) != enabled) {
        m_enabled = enabled;
        emit enabledChanged();
    }
}

void AppGroup::setSpeedLimitIn(quint32 speedLimitIn)
{
    if (m_speedLimitIn != speedLimitIn) {
        m_speedLimitIn = speedLimitIn;
        emit speedLimitInChanged();
    }
}

void AppGroup::setSpeedLimitOut(quint32 speedLimitOut)
{
    if (m_speedLimitOut != speedLimitOut) {
        m_speedLimitOut = speedLimitOut;
        emit speedLimitOutChanged();
    }
}

void AppGroup::setName(const QString &name)
{
    if (m_name != name) {
        m_name = name;
        emit nameChanged();
    }
}

void AppGroup::setBlockText(const QString &blockText)
{
    if (m_blockText != blockText) {
        m_blockText = blockText;
        emit blockTextChanged();
    }
}

void AppGroup::setAllowText(const QString &allowText)
{
    if (m_allowText != allowText) {
        m_allowText = allowText;
        emit allowTextChanged();
    }
}

QVariant AppGroup::toVariant() const
{
    QVariantMap map;

    map["speedLimitIn"] = speedLimitIn();
    map["speedLimitOut"] = speedLimitOut();

    map["name"] = name();
    map["blockText"] = blockText();
    map["allowText"] = allowText();

    return map;
}

void AppGroup::fromVariant(const QVariant &v)
{
    const QVariantMap map = v.toMap();

    m_speedLimitIn = map["speedLimitIn"].toUInt();
    m_speedLimitOut = map["speedLimitOut"].toUInt();

    m_name = map["name"].toString();
    m_blockText = map["blockText"].toString();
    m_allowText = map["allowText"].toString();
}
