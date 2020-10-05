#include "stringutil.h"

#include <QCryptographicHash>

QString StringUtil::capitalize(const QString &text)
{
    const QChar firstChar = text.at(0);

    return firstChar.toUpper() + text.mid(1);
}

QString StringUtil::cryptoHash(const QString &text)
{
    if (text.isEmpty())
        return QString();

    const QByteArray data = text.toUtf8();
    const QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Sha1);

    return QString::fromLatin1(hash.toHex());
}

int StringUtil::lineStart(const QString &text, int pos, int badPos)
{
    const int startPos = text.lastIndexOf(QLatin1Char('\n'), pos);
    return (startPos != -1) ? startPos : badPos;
}

int StringUtil::lineEnd(const QString &text, int pos, int badPos)
{
    const int endPos = text.indexOf(QLatin1Char('\n'), pos);
    return (endPos != -1) ? endPos : badPos;
}

SplitViewResult StringUtil::splitView(const QString &text, QLatin1Char sep, bool skipEmptyParts)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    const auto behavior = skipEmptyParts ? QString::SkipEmptyParts : QString::KeepEmptyParts;
    return text.splitRef(sep, behavior);
#else
    const auto behavior = skipEmptyParts ? Qt::SkipEmptyParts : Qt::KeepEmptyParts;
    return QStringView(text).split(sep, behavior);
#endif
}
