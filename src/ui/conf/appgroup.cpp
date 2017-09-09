#include "appgroup.h"

AppGroup::AppGroup(QObject *parent) :
    QObject(parent),
    m_enabled(true)
{
}

void AppGroup::setEnabled(bool enabled)
{
    if ((bool) m_enabled != enabled) {
        m_enabled = enabled;
        emit enabledChanged();
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

    //map["enabled"] = enabled();
    map["name"] = name();
    map["blockText"] = blockText();
    map["allowText"] = allowText();

    return map;
}

void AppGroup::fromVariant(const QVariant &v)
{
    const QVariantMap map = v.toMap();

    //m_enabled = map["enabled"].toBool();
    m_name = map["name"].toString();
    m_blockText = map["blockText"].toString();
    m_allowText = map["allowText"].toString();
}
