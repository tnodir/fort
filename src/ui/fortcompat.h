#ifndef FORTCOMPAT_H
#define FORTCOMPAT_H

#include <QObject>
#include <QString>

using StringView = QStringView;
using StringViewList = QList<QStringView>;
using TokenizeViewResult = QStringTokenizer<QStringView, QChar>;
#define toStringView(str) (QStringView(str))

#define mouseEventGlobalPos(event) (event)->globalPosition().toPoint()

#endif // FORTCOMPAT_H
