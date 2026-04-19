#ifndef CONFUTIL_H
#define CONFUTIL_H

#include <QObject>
#include <QRegularExpressionMatch>

#include <conf/confrulemanager.h>
#include <conf/confzonemanager.h>

class ConfUtil
{
public:
    static int ruleMaxCount();
    static int ruleGlobalMaxCount();
    static int ruleSetMaxCount();
    static int ruleDepthFilterMaxCount();
    static int ruleSetDepthMaxCount();

    static int zoneMaxCount();

    static QStringList formatUsageInAppRule(const QStringList &appNames, const QMap<Rule::RuleType, QStringList> &ruleNamesByType,
            const QString itemPrefix, const QString itemSuffix);

    static QRegularExpressionMatch matchWildcard(const QStringView path);
    static bool hasWildcard(const QString &path);

    static QString parseAppPath(const QStringView line, bool &isWild, bool &isPrefix);
};

#endif // CONFUTIL_H
