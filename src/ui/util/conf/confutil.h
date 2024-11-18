#ifndef CONFUTIL_H
#define CONFUTIL_H

#include <QObject>
#include <QRegularExpressionMatch>

class ConfUtil
{
public:
    static int ruleMaxCount();
    static int ruleSetMaxCount();
    static int ruleDepthMaxCount();
    static int ruleSetDepthMaxCount();

    static int zoneMaxCount();

    static QRegularExpressionMatch matchWildcard(const QStringView path);

    static QString parseAppPath(const QStringView line, bool &isWild, bool &isPrefix);
};

#endif // CONFUTIL_H
