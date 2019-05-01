#include "stringutil.h"

#include <QCryptographicHash>

StringUtil::StringUtil(QObject *parent) :
    QObject(parent)
{
}

QString StringUtil::capitalize(const QString &text)
{
    const QChar firstChar = text.at(0);

    return firstChar.toUpper() + text.mid(1);
}

QString StringUtil::cryptoHash(const QString &text)
{
    const QByteArray data = text.toUtf8();
    const QByteArray hash = QCryptographicHash::hash(
                data, QCryptographicHash::Sha1);

    return QString::fromLatin1(hash.toHex());
}

int StringUtil::lineStart(const QString &text, int pos)
{
    return text.lastIndexOf(QLatin1Char('\n'), pos);
}

int StringUtil::lineEnd(const QString &text, int pos)
{
    const int end = text.indexOf(QLatin1Char('\n'), pos);
    return (end > -1) ? end : text.size() - 1;
}
