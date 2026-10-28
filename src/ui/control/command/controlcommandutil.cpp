#include "controlcommandutil.h"

#include <QStringList>

#include <fortglobal.h>
#include <fortsettings.h>
#include <manager/windowmanager.h>
#include <util/variantutil.h>

using namespace Fort;

namespace {

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
