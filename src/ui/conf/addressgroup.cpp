#include "addressgroup.h"

AddressGroup::AddressGroup(QObject *parent) :
    QObject(parent),
    m_edited(false),
    m_includeAll(true),
    m_excludeAll(false),
    m_id(0)
{
}

void AddressGroup::setIncludeAll(bool includeAll)
{
    if (m_includeAll != includeAll) {
        m_includeAll = includeAll;
        emit includeAllChanged();

        setEdited(true);
    }
}

void AddressGroup::setExcludeAll(bool excludeAll)
{
    if (m_excludeAll != excludeAll) {
        m_excludeAll = excludeAll;
        emit excludeAllChanged();

        setEdited(true);
    }
}

void AddressGroup::setIncludeText(const QString &includeText)
{
    if (m_includeText != includeText) {
        m_includeText = includeText;
        emit includeTextChanged();

        setEdited(true);
    }
}

void AddressGroup::setExcludeText(const QString &excludeText)
{
    if (m_excludeText != excludeText) {
        m_excludeText = excludeText;
        emit excludeTextChanged();

        setEdited(true);
    }
}

void AddressGroup::copy(const AddressGroup &o)
{
    m_edited = false;

    m_includeAll = o.includeAll();
    m_excludeAll = o.excludeAll();

    m_id = 0;

    m_includeText = o.includeText();
    m_excludeText = o.excludeText();
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
