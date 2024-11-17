#include "dirrange.h"

DirRange::DirRange(QObject *parent) : ValueRange(parent) { }

bool DirRange::isEmpty() const
{
    return !(isIn() || isOut());
}

void DirRange::clear()
{
    ValueRange::clear();

    m_isIn = false;
    m_isOut = false;
}

QString DirRange::toText() const
{
    QStringList list;

    if (isIn()) {
        list << "IN";
    }
    if (isOut()) {
        list << "OUT";
    }

    return list.join('\n');
}

bool DirRange::fromList(const StringViewList &list, bool /*sort*/)
{
    clear();

    int lineNo = 0;
    for (const auto &line : list) {
        ++lineNo;

        const auto lineTrimmed = line.trimmed();
        if (lineTrimmed.isEmpty() || lineTrimmed.startsWith('#')) // commented line
            continue;

        if (parseDirectionLine(line) != ErrorOk) {
            appendErrorDetails(QString("line='%1'").arg(line));
            setErrorLineNo(lineNo);
            return false;
        }
    }

    return true;
}

DirRange::ParseError DirRange::parseDirectionLine(const QStringView &line)
{
    const auto name = line.toString().toUpper();

    if (name == "IN") {
        m_isIn = true;
    } else if (name == "OUT") {
        m_isOut = true;
    } else {
        return ErrorBadDirection;
    }

    return ErrorOk;
}
