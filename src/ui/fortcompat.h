#ifndef FORTCOMPAT_H
#define FORTCOMPAT_H

#include <QObject>
#include <QString>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
using StringView = QStringRef;
using StringViewList = QVector<QStringRef>;
#    define toStringView(str) (QStringRef(&str))
#else
using StringView = QStringView;
using StringViewList = QList<QStringView>;
#    define toStringView(str) (QStringView(str))
#endif

#endif // FORTCOMPAT_H
