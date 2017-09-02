#include "addressgroup.h"

AddressGroup::AddressGroup(QObject *parent) :
    QObject(parent),
    m_useAll(false)
{
}

void AddressGroup::setUseAll(bool useAll)
{
    if (m_useAll != useAll) {
        m_useAll = useAll;
        emit useAllChanged();
    }
}

void AddressGroup::setText(const QString &text)
{
    if (m_text != text) {
        m_text = text;
        emit textChanged();
    }
}

QVariant AddressGroup::toVariant() const
{
    QVariantMap map;

    map["text"] = text();

    return map;
}

void AddressGroup::fromVariant(const QVariant &v)
{
    QVariantMap map = v.toMap();

    m_text = map["text"].toString();
}
