#ifndef DIALOGUTIL_H
#define DIALOGUTIL_H

#include <QColor>
#include <QObject>

class DialogUtil
{
public:
    static QString getOpenFileName(
            const QString &title = QString(), const QString &filter = QString());
    static QStringList getOpenFileNames(
            const QString &title = QString(), const QString &filter = QString());
    static QString getSaveFileName(
            const QString &title = QString(), const QString &filter = QString());
    static QString getExistingDir(const QString &title = QString());

    static QColor getColor(const QColor &initial = Qt::white, const QString &title = QString());
};

#endif // DIALOGUTIL_H
