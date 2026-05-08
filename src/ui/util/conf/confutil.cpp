#include "confutil.h"

#include <common/fortconf.h>
#include <conf/confrulemanager.h>
#include <conf/confzonemanager.h>
#include <model/rulelistmodel.h>
#include <util/stringutil.h>

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

int ConfUtil::zoneMaxCount()
{
    return FORT_CONF_ZONE_MAX;
}

QStringList ConfUtil::formatUsageInAppRule(const QStringList &appNames, const QMap<Rule::RuleType, QStringList> &ruleNamesByType,
        const QString itemPrefix, const QString itemSuffix)
{
    QStringList output;

    if (!appNames.isEmpty()) {
        output << "<b>" + RuleListModel::tr("In Programs").toHtmlEscaped() + "</b>";
        for (const QString &appName : appNames) {
            output << itemPrefix + appName.toHtmlEscaped() + itemSuffix;
        }
        output << "";
    }

    for (auto it = ruleNamesByType.cbegin(); it != ruleNamesByType.cend(); ++it) {
        QString header;
        switch (it.key()) {
        case Rule::AppRule:
            header = RuleListModel::tr("Application Rules");
            break;
        case Rule::GlobalBeforeAppsRule:
            header = RuleListModel::tr("Global Rules, applied before App Rules");
            break;
        case Rule::GlobalAfterAppsRule:
            header = RuleListModel::tr("Global Rules, applied after App Rules");
            break;
        case Rule::PresetRule:
            header = RuleListModel::tr("Preset Rules");
            break;
        default:
            continue;
        }
        output << "<b>" + header.toHtmlEscaped() + "</b>";

        for (const QString &name : it.value()) {
            output << itemPrefix + name.toHtmlEscaped() + itemSuffix;
        }
        output << "";
    }
    return output;
}

QRegularExpressionMatch ConfUtil::matchWildcard(const QStringView path)
{
    static const QRegularExpression wildMatcher("([*?]|^\\[)");

    return StringUtil::match(wildMatcher, path);
}

bool ConfUtil::hasWildcard(const QString &path)
{
    return matchWildcard(path).hasMatch();
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
        const auto wildMatch = matchWildcard(path);
        if (wildMatch.hasMatch()) {
            if (wildMatch.capturedStart() == path.size() - 2
                    && path.endsWith(QLatin1String("**"))) {
                path.chop(2);
                isPrefix = true;
            } else {
                isWild = true;
            }
        }
    }

    return path.toString();
}
