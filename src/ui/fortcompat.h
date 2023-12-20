#ifndef FORTCOMPAT_H
#define FORTCOMPAT_H

#include <QObject>
#include <QString>

using StringView = QStringView;
using StringViewList = QList<QStringView>;
using TokenizeViewResult = QStringTokenizer<QStringView, QChar>;
#define toStringView(str) QStringView(str)

#define mouseEventGlobalPos(event) (event)->globalPosition().toPoint()

#define asConst(t) std::as_const(t)

#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
#    define matchRegExp(re, s) (re).matchView(s)
#else
#    define matchRegExp(re, s) (re).match(s)
#endif

#endif // FORTCOMPAT_H
