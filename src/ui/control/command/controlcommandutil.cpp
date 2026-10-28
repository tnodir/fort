#include "controlcommandutil.h"

#include <QDir>
#include <QStringList>

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

bool parseFieldArg(const QString &arg, QVariantMap &fields, QString &errorMessage)
{
    const int sepIndex = arg.indexOf('=');
    const auto it = fields.find(arg.left(sepIndex));

    if (sepIndex <= 0 || it == fields.end()) {
        errorMessage = "Invalid field: " + arg;
        return false;
    }

    if (!VariantUtil::parseValue(arg.mid(sepIndex + 1), it.value())) {
        return false;
    }

    return true;
}

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

QString ControlCommandUtil::invalidValueText(const QString &key, const QString &text)
{
    return "Invalid value: " + key + '=' + text;
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

        list.append(key + '=' + VariantUtil::toText(value));
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
