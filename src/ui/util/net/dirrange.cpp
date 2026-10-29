#include "dirrange.h"

#include <common/fortconf.h>

#include <util/conf/confdata.h>

DirRange::DirRange(QObject *parent) : TextRange(parent) { }

bool DirRange::isEmpty() const
{
    return !(isIn() || isOut());
}

void DirRange::clear()
{
    TextRange::clear();

    m_isIn = false;
    m_isOut = false;
}

void DirRange::toList(QStringList &list) const
{
    if (isIn()) {
        list << directionName(/*inbound=*/true).toUpper();
    }
    if (isOut()) {
        list << directionName(/*inbound=*/false).toUpper();
    }
}

QString DirRange::directionName(bool inbound)
{
    return inbound ? "In" : "Out";
}

quint8 DirRange::directionIdByName(const QString &name)
{
    if (name.compare(directionName(/*inbound=*/true), Qt::CaseInsensitive) == 0)
        return FORT_RULE_FILTER_DIRECTION_IN;

    if (name.compare(directionName(/*inbound=*/false), Qt::CaseInsensitive) == 0)
        return FORT_RULE_FILTER_DIRECTION_OUT;

    return 0;
}

TextRange::ParseError DirRange::parseText(const QString &text)
{
    const quint8 directionId = directionIdByName(text);
    if (directionId == 0)
        return ErrorBadText;

    if (directionId == FORT_RULE_FILTER_DIRECTION_IN) {
        m_isIn = true;
    } else {
        m_isOut = true;
    }

    return ErrorOk;
}

void DirRange::write(ConfData &confData) const
{
    confData.writeDirRange(*this);
}
