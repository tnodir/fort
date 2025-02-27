#ifndef CONFUTIL_H
#define CONFUTIL_H

#include <QObject>
#include <QRegularExpressionMatch>

class ConfUtil
{
public:
    static int zoneMaxCount();

    static int ruleMaxCount();
    static int ruleGlobalMaxCount();
    static int ruleSetMaxCount();
    static int ruleDepthFilterMaxCount();
    static int ruleSetDepthMaxCount();

    static int groupMaxCount();

    static QRegularExpressionMatch matchWildcard(const QStringView path);
    static bool hasWildcard(const QStringView path);

    static QString parseAppPath(const QStringView line, bool &isWild, bool &isPrefix);
};

#endif // CONFUTIL_H
