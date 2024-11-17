#include "valuerange.h"

#include <util/stringutil.h>

ValueRange::ValueRange(QObject *parent) : QObject(parent) { }

void ValueRange::clear()
{
    m_errorLineNo = 0;
    m_errorMessage.clear();
    m_errorDetails.clear();
}

QString ValueRange::toText() const
{
    QStringList list;
    toList(list);
    return list.join('\n') + '\n';
}

bool ValueRange::fromText(const QString &text)
{
    const auto list = StringUtil::splitView(text, QLatin1Char('\n'));
    return fromList(list);
}

QString ValueRange::errorLineAndMessageDetails() const
{
    return tr("Error at line %1: %2 (%3)")
            .arg(QString::number(errorLineNo()), errorMessage(), errorDetails());
}

void ValueRange::appendErrorDetails(const QString &errorDetails)
{
    m_errorDetails += (m_errorDetails.isEmpty() ? QString() : QString(' ')) + errorDetails;
}
