#include "confutil.h"

#include <common/fortconf.h>

#include <conf/rule.h>
#include <util/net/actionrange.h>
#include <util/net/optionrange.h>
#include <util/stringutil.h>

#include "filterline.h"

namespace {

// The Terminating Rule's line follows it in the Program's filters' text
inline constexpr char terminatingRuleComment[] = "# Terminating";

void parseFilterRange(const RuleFilter *filter, ValueRange &range)
{
    if (filter) {
        range.fromList(filter->values);
    }
}

// e.g. "Act(Block):Opt(Alert)"
void parseTerminatingRuleLine(const QString &text, Rule &rule)
{
    FilterLine line(text);
    line.parse();

    ActionRange actionRange;
    parseFilterRange(line.filter(FORT_RULE_FILTER_TYPE_ACTION), actionRange);

    OptionRange optionRange;
    parseFilterRange(line.filter(FORT_RULE_FILTER_TYPE_OPTION), optionRange);

    const quint8 actionTypeId = actionRange.actionTypeId();

    rule.terminate = (actionTypeId != 0);
    rule.terminateBlocked = (actionTypeId != FORT_RULE_FILTER_ACTION_ALLOW);
    rule.terminateDrop = (actionTypeId == FORT_RULE_FILTER_ACTION_DROP);
    rule.terminateAlert = (optionRange.optionTypeIds() & FORT_CONN_FILTER_RESULT_CONN_ALERT) != 0;
}

QString terminatingRuleLine(const Rule &rule)
{
    const QString action =
            !rule.terminateBlocked ? "Allow" : (rule.terminateDrop ? "Drop" : "Block");

    FilterLineText line;
    line.addFilter(FORT_RULE_FILTER_TYPE_ACTION, action);
    line.addFilter(FORT_RULE_FILTER_TYPE_OPTION, rule.terminateAlert ? "Alert" : QString());

    return line.text();
}

}

int ConfUtil::zoneMaxCount()
{
    return FORT_CONF_ZONE_MAX;
}

int ConfUtil::groupMaxCount()
{
    return FORT_CONF_GROUP_MAX;
}

int ConfUtil::speedLimitMaxCount()
{
    return FORT_CONF_SPEED_LIMIT_MAX;
}

int ConfUtil::timePeriodMaxCount()
{
    return 64; // the UI only folds them into the Groups' and Speed Limits' flags
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

void ConfUtil::parseAppFiltersText(const QString &filtersText, Rule &rule)
{
    const QStringList lines = filtersText.split('\n', Qt::SkipEmptyParts);

    const int terminatingIndex = lines.indexOf(terminatingRuleComment);

    rule.ruleText = lines.mid(0, terminatingIndex).join('\n'); // -1: all lines

    parseTerminatingRuleLine(
            (terminatingIndex >= 0) ? lines.value(terminatingIndex + 1) : QString(), rule);
}

QString ConfUtil::appFiltersText(const Rule &rule)
{
    QStringList lines;

    if (!rule.ruleText.isEmpty()) {
        lines << rule.ruleText;
    }

    if (rule.terminate) {
        lines << terminatingRuleComment << terminatingRuleLine(rule);
    }

    return lines.join('\n');
}
