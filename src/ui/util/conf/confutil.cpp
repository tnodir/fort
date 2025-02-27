#include "confutil.h"

#include <common/fortconf.h>

#include <util/stringutil.h>

int ConfUtil::zoneMaxCount()
{
    return FORT_CONF_ZONE_MAX;
}

int ConfUtil::ruleMaxCount()
{
    return FORT_CONF_RULE_MAX;
}

int ConfUtil::ruleGlobalMaxCount()
{
    return FORT_CONF_RULE_GLOBAL_MAX;
}

int ConfUtil::ruleSetMaxCount()
{
    return FORT_CONF_RULE_SET_MAX;
}

int ConfUtil::ruleDepthFilterMaxCount()
{
    return FORT_CONF_RULE_FILTER_DEPTH_MAX;
}

int ConfUtil::ruleSetDepthMaxCount()
{
    return FORT_CONF_RULE_SET_DEPTH_MAX;
}

int ConfUtil::groupMaxCount()
{
    return FORT_CONF_GROUP_MAX;
}

int ConfUtil::wildcardPos(const QStringView path)
{
    if (path.startsWith('['))
        return 0;

    static const QRegularExpression wildMatcher("([*?])");

    return StringUtil::match(wildMatcher, path).capturedStart();
}

bool ConfUtil::hasWildcard(const QString &path)
{
    return wildcardPos(path) >= 0;
}

QString ConfUtil::parseAppPath(const QStringView line, bool &isWild, bool &isPrefix)
{
    auto path = line;
    if (path.startsWith('"') && path.endsWith('"')) {
        path = path.mid(1, path.size() - 2);
    }

    if (path.isEmpty())
        return QString();

    if (path.startsWith('^')) {
        path = path.mid(1);
        isWild = true;
    } else {
        const auto wildPos = wildcardPos(path);
        if (wildPos >= 0) {
            if (wildPos == path.size() - 2 && path.endsWith(QLatin1String("**"))) {
                path.chop(2);
                isPrefix = true;
            } else {
                isWild = true;
            }
        }
    }

    return path.toString();
}
