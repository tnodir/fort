#include "addressgroup.h"

AddressGroup::AddressGroup(QObject *parent) :
    QObject(parent),
    m_edited(false),
    m_includeAll(true),
    m_excludeAll(false)
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
    m_edited = o.edited();

    m_includeAll = o.includeAll();
    m_excludeAll = o.excludeAll();

    m_id = o.id();

    m_includeText = o.includeText();
    m_excludeText = o.excludeText();
}
