#include "stringutil.h"

StringUtil::StringUtil(QObject *parent) :
    QObject(parent)
{
}

QString StringUtil::capitalize(const QString &text)
{
    const QChar firstChar = text.at(0);

    return firstChar.toUpper() + text.mid(1);
}
