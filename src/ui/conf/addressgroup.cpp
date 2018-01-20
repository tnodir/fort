#include "addressgroup.h"

AddressGroup::AddressGroup(QObject *parent) :
    QObject(parent),
    m_includeAll(true),
    m_excludeAll(false)
{
}

void AddressGroup::setIncludeAll(bool includeAll)
{
    if (m_includeAll != includeAll) {
        m_includeAll = includeAll;
        emit includeAllChanged();
    }
}

void AddressGroup::setExcludeAll(bool excludeAll)
{
    if (m_excludeAll != excludeAll) {
        m_excludeAll = excludeAll;
        emit excludeAllChanged();
    }
}

void AddressGroup::setIncludeText(const QString &includeText)
{
    if (m_includeText != includeText) {
        m_includeText = includeText;
        emit includeTextChanged();
    }
}

void AddressGroup::setExcludeText(const QString &excludeText)
{
    if (m_excludeText != excludeText) {
        m_excludeText = excludeText;
        emit excludeTextChanged();
    }
}

QVariant AddressGroup::toVariant() const
{
    QVariantMap map;

    map["includeAll"] = includeAll();
    map["excludeAll"] = excludeAll();

    map["includeText"] = includeText();
    map["excludeText"] = excludeText();

    return map;
}

void AddressGroup::fromVariant(const QVariant &v)
{
    const QVariantMap map = v.toMap();

    m_includeAll = map["includeAll"].toBool();
    m_excludeAll = map["excludeAll"].toBool();

    m_includeText = map["includeText"].toString();
    m_excludeText = map["excludeText"].toString();
}
