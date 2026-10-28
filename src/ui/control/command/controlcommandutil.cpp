#include "controlcommandutil.h"

#include <QDir>
#include <QStringList>

#include <algorithm>

#include <conf/confrulemanager.h>
#include <conf/rule.h>
#include <fortglobal.h>
#include <fortsettings.h>
#include <manager/windowmanager.h>
#include <util/conf/confbuffer.h>
#include <util/fileutil.h>
#include <util/variantutil.h>

using namespace Fort;

namespace {

// The <file-path> argument's index, after the action and its object
inline constexpr int clientFileArgIndex = 2;

QString clientFilePath(const QVariantList &args)
{
    // The working directory is changed to the program's one on startup
    const QDir currentDir(settings()->initialCurrentPath());

    return currentDir.absoluteFilePath(args.value(clientFileArgIndex).toString());
}

bool parseValue(const QString &text, QVariant &value)
{
    if (value.metaType().id() == QMetaType::QVariantList) {
        return ControlCommandUtil::parseIds(text, value);
    }

    return VariantUtil::parseValue(text, value);
}

QString argKey(const QString &arg)
{
    const int sepIndex = arg.indexOf('=');

    return (sepIndex > 0) ? arg.left(sepIndex) : QString();
}

bool parseFieldArg(const QString &arg, QVariantMap &fields, QString &errorMessage)
{
    const QString key = argKey(arg);
    const auto it = fields.find(key);

    if (key.isEmpty() || it == fields.end()) {
        errorMessage = "Invalid field: " + arg;
        return false;
    }

    const QString text = arg.mid(key.size() + 1);

    if (!parseValue(text, it.value())) {
        errorMessage = ControlCommandUtil::invalidValueText(key, text);
        return false;
    }

    return true;
}

QString stringText(QString text)
{
    const bool isQuoted =
            std::any_of(text.begin(), text.end(), [](QChar c) { return c.isSpace() || c == '"'; });
    if (!isQuoted)
        return text;

    return '"' + text.replace('"', "\"\"") + '"';
}

QString valueText(const QVariant &value)
{
    switch (value.metaType().id()) {
    case QMetaType::QVariantList: {
        return ControlCommandUtil::idsText(value);
    }
    case QMetaType::QString: {
        return stringText(value.toString());
    }
    default:
        return VariantUtil::toText(value);
    }
}

}

void ControlCommandUtil::setErrorResult(ProcessCommandResult &r, const QString &errorMessage)
{
    r.commandResult = Control::CommandResultError;
    r.errorMessage = errorMessage;
}

bool ControlCommandUtil::checkCommandActionPassword(
        ProcessCommandResult &r, quint32 action, quint32 passwordNotRequiredActions)
{
    if ((action & passwordNotRequiredActions) != 0)
        return true;

    if (r.disableCmdLine) {
        r.errorMessage = "Command line disabled";
        return false;
    }

    if (!windowManager()->checkPassword()) {
        r.errorMessage = "Password required";
        return false;
    }

    settings()->setPasswordTemporaryChecked(true);

    return true;
}

void ControlCommandUtil::uncheckCommandActionPassword()
{
    settings()->setPasswordTemporaryChecked(false);
}

bool ControlCommandUtil::parseFieldArgs(
        const QVariantList &args, int index, QVariantMap &fields, QString &errorMessage)
{
    for (int i = index; i < args.size(); ++i) {
        if (!parseFieldArg(args[i].toString(), fields, errorMessage))
            return false;
    }

    return true;
}

bool ControlCommandUtil::isFieldArg(const QString &arg, const QVariantMap &fields)
{
    const QString key = argKey(arg);

    return !key.isEmpty() && fields.contains(key);
}

QString ControlCommandUtil::invalidValueText(const QString &key, const QString &text)
{
    return "Invalid value: " + key + '=' + text;
}

int ControlCommandUtil::nameIndex(const QVariantMap &fields, const QString &key,
        const QStringList &names, QString &errorMessage)
{
    const QString text = fields[key].toString();
    const int index = names.indexOf(text);

    if (index < 0) {
        errorMessage = invalidValueText(key, text);
    }

    return index;
}

bool ControlCommandUtil::parseIds(const QString &text, QVariant &ids)
{
    QVariantList list;

    for (const auto &idText : text.split(',', Qt::SkipEmptyParts)) {
        const int id = idText.toInt();
        if (id < 1 || id > 32)
            return false;

        list.append(id);
    }

    ids = list;

    return true;
}

QString ControlCommandUtil::idsText(const QVariant &ids)
{
    return ids.toStringList().join(',');
}

QVariant ControlCommandUtil::maskIds(quint32 mask)
{
    QVariantList ids;

    for (int id = 1; mask != 0; ++id, mask >>= 1) {
        if ((mask & 1) != 0) {
            ids.append(id);
        }
    }

    return ids;
}

quint32 ControlCommandUtil::idsMask(const QVariant &ids)
{
    quint32 mask = 0;

    for (const QVariant &id : ids.toList()) {
        mask |= quint32(1) << (id.toInt() - 1);
    }

    return mask;
}

QString ControlCommandUtil::fieldsToText(const QVariantMap &fields)
{
    QStringList list;

#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    for (const auto &[key, value] : fields.asKeyValueRange()) {
#else
    for (auto it = fields.constBegin(); it != fields.constEnd(); ++it) {
        const QString &key = it.key();
        const QVariant &value = it.value();
#endif

        list.append(key + '=' + valueText(value));
    }

    return list.join(' ');
}

bool ControlCommandUtil::readClientFileArg(QVariantList &args, ProcessCommandResult &r)
{
    const QString filePath = clientFilePath(args);

    if (!FileUtil::fileExists(filePath)) {
        r.errorMessage = "File not found";
        return false;
    }

    // A directory or an unreadable file
    QString text;
    if (!FileUtil::readFile(filePath, text)) {
        r.errorMessage = "Cannot read the file";
        return false;
    }

    // The texts' lines are split by '\n'
    args[clientFileArgIndex] = text.replace("\r\n", "\n");

    return true;
}

bool ControlCommandUtil::writeClientFileArg(const QVariantList &args, ProcessCommandResult &r)
{
    if (r.commandResult == Control::CommandResultError)
        return true;

    if (!FileUtil::writeFile(clientFilePath(args), r.errorMessage)) {
        r.errorMessage = "Cannot write the file";
        return false;
    }

    r.errorMessage.clear();

    return true;
}

bool ControlCommandUtil::findRuleByName(const QString &ruleName, Rule &rule, QString &errorMessage)
{
    const auto ruleList = confRuleManager()->getRulesByName(ruleName);

    if (ruleList.size() != 1) {
        errorMessage = ruleList.isEmpty() ? "Rule not found" : "Rule name is ambiguous";
        return false;
    }

    rule = ruleList.first();

    return true;
}

bool ControlCommandUtil::findRuleIdByName(
        const QString &ruleName, quint16 &ruleId, QString &errorMessage)
{
    Rule rule;

    if (!ruleName.isEmpty() && !findRuleByName(ruleName, rule, errorMessage))
        return false;

    ruleId = rule.ruleId;

    return true;
}

bool ControlCommandUtil::validateRuleText(ProcessCommandResult &r, const QString &ruleText)
{
    if (ruleText.isEmpty())
        return true;

    ConfBuffer confBuf;
    if (!confBuf.validateRuleText(ruleText)) {
        r.errorMessage = confBuf.errorMessage();
        return false;
    }

    return true;
}
