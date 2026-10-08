#ifndef CONFUTIL_H
#define CONFUTIL_H

#include <QObject>

class Rule;

class ConfUtil
{
public:
    static int zoneMaxCount();
    static int groupMaxCount();
    static int speedLimitMaxCount();
    static int timePeriodMaxCount();

    static int ruleMaxCount();
    static int ruleGlobalMaxCount();
    static int ruleSetMaxCount();
    static int ruleDepthFilterMaxCount();
    static int ruleSetDepthMaxCount();

    static int wildcardPos(const QStringView path);
    static bool hasWildcard(const QString &path);

    static QString parseAppPath(const QStringView line, bool &isWild, bool &isPrefix);

    // The Program's Network Filters' text: the filters' lines, then the Terminating Rule's line
    // after the "# Terminating" comment line, e.g. "Act(Block):Opt(Alert)".
    // Its Rule has the filters' lines as the text and the Terminating Rule's flags.
    static void parseAppFiltersText(const QString &filtersText, Rule &rule);
    static QString appFiltersText(const Rule &rule);
};

#endif // CONFUTIL_H
